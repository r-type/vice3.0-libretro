/* Copyright (C) 2018 
 *
 * Permission is hereby granted, free of charge,
 * to any person obtaining a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "retro_disk_control.h"
#include "retro_strings.h"
#include "retro_files.h"
#include "file/file_path.h"
#include "libretro.h"
#include "libretro-core.h"
#include "retroglue.h"

#include "archdep.h"
#include "attach.h"
#include "drive.h"
#include "tape.h"
#include "resources.h"

#ifdef __XSCPU64__
int tape_deinstall(void)
{
    return 0;
}
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define COMMENT '#'
#define M3U_SPECIAL_COMMAND "#COMMAND:"
#define M3U_NONSTD_LABEL    "#LABEL:"
#define M3U_EXTSTD_LABEL    "#EXTINF:" /* Title should be following comma */
#define VFL_UNIT_ENTRY      "UNIT "

#define MAX_LABEL_LEN     27 /* Max of disk (27) and tape (24) */

#define D64_NAME_POS      0x16590 /* Sector 18/0, offset 0x90 */
#define D64_FULL_NAME_LEN 27      /* Including id, dos version and paddings */
#define D64_NAME_LEN      16

#define T64_NAME_POS      40
#define T64_NAME_LEN      24

#undef  DISK_LABEL_RELAXED         /* Use label even if it doesn't look sane (mostly for testing) */
#undef  DISK_LABEL_FORBID_SHIFTED  /* Reject label if has shifted chars */

static const char flip_file_header[] = "# Vice fliplist file";

extern char retro_save_directory[RETRO_PATH_MAX];
extern char retro_temp_directory[RETRO_PATH_MAX];
extern bool retro_disk_set_image_index(unsigned index);
extern int runstate;

extern retro_log_printf_t log_cb;

/* Return the directory name of 'filename' without trailing separator.
 * Allocates returned string. */
static char* dirname_int(const char* filename)
{
    if (filename == NULL)
        return NULL;

    /* Find last separator */
    char* right = find_last_slash(filename);
    if (right)
        return strleft(filename, right - filename);

    /* Not found */
    return NULL;
}

/* Add known disk labels from conversion tools, famous collectors etc. */
static const char const* rude_words[] = {
    "semprini",
    "ass presents",
};

/* Filter out this annoying "ASS PRESENTS" */
static bool is_ugly(const char* label)
{
    size_t i;
    for (i = 0; i < sizeof(rude_words) / sizeof(rude_words[0]); ++i)
    {
        if (strcasestr(label, rude_words[i]))
            return true;
    }
    return false;
}

#define PETSCII_SHIFTED_BIT       0x20

#define PETSCII_SPACE             0x20
#define PETSCII_NBSP              0xA0
#define PETSCII_UNSHIFTED_A       0x40
#define PETSCII_UNSHIFTED_Z       0x5A
#define PETSCII_SHIFTED_A         0x60
#define PETSCII_SHIFTED_Z         0x7A

/* Try to read disk or tape name from image
 * Allocates returned string */
static char* get_label(const char* filename)
{
    unsigned char label[MAX_LABEL_LEN + 1];
    bool have_disk_label = false;
    bool have_tape_label = false;
    bool have_shifted = false;
    int i;

    label[0] = '\0';
    /* Disk image which we can read name from */
    if (strendswith(filename, "d64") || strendswith(filename, "d71"))
    {
        FILE* fd = fopen(filename, "rb");

        if (fd != NULL)
        {
            if (fseek(fd, D64_NAME_POS, SEEK_SET) == 0
                && fread(label, D64_FULL_NAME_LEN, 1, fd) == 1)
            {
                label[D64_FULL_NAME_LEN] = '\0';
                have_disk_label = true;
            }
            fclose(fd);
        }
    }

    /* Tape image which we can read name from */
    if (strendswith(filename, "t64"))
    {
        FILE* fd = fopen(filename, "rb");

        if (fd != NULL)
        {
            if (fseek(fd, T64_NAME_POS, SEEK_SET) == 0
                && fread(label, T64_NAME_LEN, 1, fd) == 1)
            {
                label[T64_NAME_LEN] = '\0';
                have_tape_label = true;
            }
            fclose(fd);
        }
    }

    /* Special processing for disk label - sanity check and trimming */
    if (have_disk_label)
    {
#if !defined(DISK_LABEL_RELAXED)
        /* Chack if all characters in disk name and padding areas look valid
         * that may be too picky, but it's better to show nothing than garbage */
        for (i = 0; i < D64_FULL_NAME_LEN; ++i)
        {
            unsigned char c = label[i];
            if (c != PETSCII_NBSP && (c < PETSCII_SPACE || c > PETSCII_SHIFTED_Z))
            {
                return strdup((char*)label);
            }
        }
#endif
        label[D64_NAME_LEN - 1] = '\0';
    }

    /* Remove trailing spaces */
    i = strlen((char*)label) - 1;
    while (i > 0 && (label[i] == PETSCII_NBSP || label[i] == PETSCII_SPACE))
        label[i--] = '\0';

    /* Replace other nbsp with spaces */
    while (i > 0)
    {
        if (label[i] == PETSCII_NBSP)
            label[i] = ' ';
        --i;
    }

    /* Check for shifted chars */
    for (i=0; label[i] != '\0'; ++i)
    {
        unsigned char c = label[i];
        if (c >= PETSCII_SHIFTED_A)
        {
#if defined(DISK_LABEL_FORBID_SHIFTED)
            return strdup((char*)label);
#endif
            /* Have shifted chars */
            have_shifted = true;
            break;
        }
    }

    int mode = disk_label_mode;
    if (have_shifted 
        && (mode == DISK_LABEL_MODE_ASCII_OR_UPPERCASE || mode == DISK_LABEL_MODE_ASCII_OR_CAMELCASE))
    {
        mode = DISK_LABEL_MODE_ASCII;
    }

    bool was_space = true;
    /* Convert petscii to ascii */
    for (i=0; label[i] != '\0'; ++i)
    {
        unsigned char c = label[i];
        if (c == PETSCII_SPACE)
        {
            was_space = true;
        }
        else
        {
            if (c >= PETSCII_UNSHIFTED_A && c <= PETSCII_UNSHIFTED_Z)
            {
                /* Unshifted - starts as uppercase, toggles to lowercase */
                if (mode == DISK_LABEL_MODE_ASCII || mode == DISK_LABEL_MODE_LOWERCASE
                    || (mode == DISK_LABEL_MODE_ASCII_OR_CAMELCASE && !was_space))
                {
                    label[i] = c ^ PETSCII_SHIFTED_BIT;
                }
            }
            else if (c >= PETSCII_SHIFTED_A && c <= PETSCII_SHIFTED_Z)
            {
                /* Shifted - starts as lowercase, toggles to uppercase */
                if (mode == DISK_LABEL_MODE_ASCII || mode == DISK_LABEL_MODE_UPPERCASE)
                {
                    label[i] = c ^ PETSCII_SHIFTED_BIT;
                }
            }
            was_space = false;
        }
    }

    if (is_ugly((char*)label))
    {
        return strdup((char*)"");
    }

    return strdup((char*)label);
}

/* Search for image file relative to M3U
 * Allocates returned string */
static char* m3u_search_file(const char* basedir, const char* dskName)
{
    /* If basedir was provided and path is relative, search relative to M3U file location */
    if (basedir != NULL && !path_is_absolute(dskName))
    {
        /* Join basedir and dskName */
        char* dskPath = path_join_dup(basedir, dskName);

        /* Verify if this item is a relative filename (append it to the m3u path) */
        if (file_exists(dskPath))
        {
            /* Return */
            return dskPath;
        }
        free(dskPath);
    }

    /* Verify if this item is an absolute pathname (or the file is in working dir) */
    if (file_exists(dskName))
    {
        /* Copy and return */
        return strdup(dskName);
    }

    /* File not found */
    return NULL;
}

void dc_reset(dc_storage* dc)
{
    /* Verify */
    if (dc == NULL)
        return;

    /* Clean the command */
    free(dc->command);
    dc->command = NULL;

    /* Clean the struct */
    for (unsigned i=0; i < dc->count; i++)
    {
        free(dc->files[i]);
        dc->files[i] = NULL;
        free(dc->labels[i]);
        dc->labels[i] = NULL;
        free(dc->names[i]);
        dc->names[i] = NULL;

        dc->types[i] = DC_IMAGE_TYPE_NONE;
    }

    dc->unit = 0;
    dc->count = 0;
    dc->index = 0;
    dc->eject_state = true;
    dc->replace = false;
}

dc_storage* dc_create(void)
{
    /* Initialize the struct */
    dc_storage* dc = NULL;

    if((dc = malloc(sizeof(dc_storage))) != NULL)
    {
        dc->unit = 0;
        dc->count = 0;
        dc->index = 0;
        dc->eject_state = true;
        dc->replace = false;
        dc->command = NULL;
        for(int i = 0; i < DC_MAX_SIZE; i++)
        {
            dc->files[i]  = NULL;
            dc->labels[i] = NULL;
            dc->names[i]  = NULL;
            dc->types[i]  = DC_IMAGE_TYPE_NONE;
        }
    }

    return dc;
}

bool dc_add_file_int(dc_storage* dc, char* filename, char* label, char* name)
{
    /* Verify */
    if (filename == NULL || dc == NULL || dc->count > DC_MAX_SIZE)
    {
        free(filename);
        free(label);
        free(name);

        return false;
    }

    /* Add the file */
    dc->count++;
    dc->files[dc->count-1]  = filename;
    dc->labels[dc->count-1] = label;
    dc->names[dc->count-1]  = name;
    dc->types[dc->count-1]  = dc_get_image_type(filename);
    return true;
}

bool dc_add_file(dc_storage* dc, const char* filename)
{
    /* Verify */
    if (dc == NULL || string_is_empty(filename))
        return false;

    /* Determine if tape or disk fliplist from first entry */
    if (dc->unit != -1)
    {
        if (dc_get_image_type(dc->files[0]) == DC_IMAGE_TYPE_TAPE)
            dc->unit = 1;
        else if (dc_get_image_type(dc->files[0]) == DC_IMAGE_TYPE_FLOPPY)
            dc->unit = 8;
        else if (dc_get_image_type(dc->files[0]) == DC_IMAGE_TYPE_MEM)
            dc->unit = 0;
        else
            dc->unit = 8;
    }

    /* Get 'name' - just the filename without extension
     * > It would be nice to make use of the image label
     *   here, but this often bears no relationship to
     *   the actual file content... */
    char image_name[512];
    image_name[0] = '\0';
    fill_short_pathname_representation(image_name, filename, sizeof(image_name));

    /* Copy and return */
    return dc_add_file_int(dc, strdup(filename), get_label(filename), strdup(image_name));
}

bool dc_remove_file(dc_storage* dc, int index)
{
    if (dc == NULL)
        return false;

    if (index < 0 || index >= dc->count)
        return false;

    /* "If ptr is a null pointer, no action occurs" */
    free(dc->files[index]);
    dc->files[index] = NULL;
    free(dc->labels[index]);
    dc->labels[index] = NULL;
    free(dc->names[index]);
    dc->names[index] = NULL;
    dc->types[index] = DC_IMAGE_TYPE_NONE;

    /* Shift all entries after index one slot up */
    if (index != dc->count - 1)
    {
        memmove(dc->files + index, dc->files + index + 1, (dc->count - 1 - index) * sizeof(dc->files[0]));
        memmove(dc->labels + index, dc->labels + index + 1, (dc->count - 1 - index) * sizeof(dc->labels[0]));
        memmove(dc->names + index, dc->names + index + 1, (dc->count - 1 - index) * sizeof(dc->names[0]));
    }

    dc->count--;

    /* Reset fliplist unit after removing last entry */
    if (dc->count == 0)
    {
        dc->unit = 0;
    }

    return true;
}

bool dc_replace_file(dc_storage* dc, int index, const char* filename)
{
    if (dc == NULL)
        return false;

    if (index < 0 || index >= dc->count)
        return false;

    /* "If ptr is a null pointer, no action occurs" */
    free(dc->files[index]);
    dc->files[index] = NULL;
    free(dc->labels[index]);
    dc->labels[index] = NULL;
    free(dc->names[index]);
    dc->names[index] = NULL;
    dc->types[index] = DC_IMAGE_TYPE_NONE;

    if (filename == NULL)
    {
        dc_remove_file(dc, index);
    }
    else
    {
        dc->replace = false;

        char full_path_replace[RETRO_PATH_MAX] = {0};
        strcpy(full_path_replace, (char*)filename);

        /* ZIP + NIB vars, use the same temp directory for single NIBs */
        char zip_basename[RETRO_PATH_MAX] = {0};
        snprintf(zip_basename, sizeof(zip_basename), "%s", path_basename(full_path_replace));
        snprintf(zip_basename, sizeof(zip_basename), "%s", path_remove_extension(zip_basename));
        snprintf(retro_temp_directory, sizeof(retro_temp_directory), "%s%s%s", retro_save_directory, FSDEV_DIR_SEP_STR, "TEMP");

        char nib_input[RETRO_PATH_MAX] = {0};
        char nib_output[RETRO_PATH_MAX] = {0};

        /* NIB convert to G64 */
        if (dc_get_image_type(full_path_replace) == DC_IMAGE_TYPE_NIBBLER)
        {
            snprintf(nib_input, sizeof(nib_input), "%s", full_path_replace);
            snprintf(nib_output, sizeof(nib_output), "%s%s%s.g64", retro_temp_directory, FSDEV_DIR_SEP_STR, zip_basename);
            path_mkdir(retro_temp_directory);
            nib_convert(nib_input, nib_output);
            snprintf(full_path_replace, sizeof(full_path_replace), "%s", nib_output);
        }

        /* ZIP */
        if (strendswith(full_path_replace, "zip"))
        {
            path_mkdir(retro_temp_directory);
            zip_uncompress(full_path_replace, retro_temp_directory, NULL);

            /* Default to directory mode */
            int zip_mode = 0;
            snprintf(full_path_replace, sizeof(full_path_replace), "%s", retro_temp_directory);

            FILE *zip_m3u;
            char zip_m3u_list[DC_MAX_SIZE][RETRO_PATH_MAX] = {0};
            char zip_m3u_path[RETRO_PATH_MAX] = {0};
            snprintf(zip_m3u_path, sizeof(zip_m3u_path), "%s%s%s.m3u", retro_temp_directory, FSDEV_DIR_SEP_STR, zip_basename);
            int zip_m3u_num = 0;

            DIR *zip_dir;
            struct dirent *zip_dirp;

            /* Convert all NIBs to G64 */
            zip_dir = opendir(retro_temp_directory);
            while ((zip_dirp = readdir(zip_dir)) != NULL)
            {
                if (dc_get_image_type(zip_dirp->d_name) == DC_IMAGE_TYPE_NIBBLER)
                {
                    snprintf(nib_input, sizeof(nib_input), "%s%s%s", retro_temp_directory, FSDEV_DIR_SEP_STR, zip_dirp->d_name);
                    snprintf(nib_output, sizeof(nib_output), "%s%s%s.g64", retro_temp_directory, FSDEV_DIR_SEP_STR, path_remove_extension(zip_dirp->d_name));
                    nib_convert(nib_input, nib_output);
                }
            }
            closedir(zip_dir);

            zip_dir = opendir(retro_temp_directory);
            while ((zip_dirp = readdir(zip_dir)) != NULL)
            {
                if (zip_dirp->d_name[0] == '.' || strendswith(zip_dirp->d_name, "m3u") || zip_mode > 1)
                    continue;

                /* Multi file mode, generate playlist */
                if (dc_get_image_type(zip_dirp->d_name) == DC_IMAGE_TYPE_FLOPPY
                 || dc_get_image_type(zip_dirp->d_name) == DC_IMAGE_TYPE_TAPE
                 || dc_get_image_type(zip_dirp->d_name) == DC_IMAGE_TYPE_MEM
                )
                {
                    zip_mode = 1;
                    zip_m3u_num++;
                    snprintf(zip_m3u_list[zip_m3u_num-1], RETRO_PATH_MAX, "%s", zip_dirp->d_name);
                }
            }
            closedir(zip_dir);

            switch (zip_mode)
            {
                case 0: /* Extracted path */
                    dc_reset(dc);
                    return true;
                    break;
                case 1: /* Generated playlist */
                    if (zip_m3u_num == 1)
                    {
                        snprintf(full_path_replace, sizeof(full_path_replace), "%s%s%s", retro_temp_directory, FSDEV_DIR_SEP_STR, zip_m3u_list[0]);
                    }
                    else
                    {
                        zip_m3u = fopen(zip_m3u_path, "w");
                        qsort(zip_m3u_list, zip_m3u_num, RETRO_PATH_MAX, qstrcmp);
                        for (int l = 0; l < zip_m3u_num; l++)
                            fprintf(zip_m3u, "%s\n", zip_m3u_list[l]);
                        fclose(zip_m3u);
                        snprintf(full_path_replace, sizeof(full_path_replace), "%s", zip_m3u_path);
                    }
                    break;
            }
        }

        /* M3U replace */
        if (strendswith(full_path_replace, ".m3u"))
        {
            /* Parse the M3U file */
            dc_parse_m3u(dc, full_path_replace);

            /* Some debugging */
            log_cb(RETRO_LOG_INFO, "M3U/VFL parsed, %d file(s) found\n", dc->count);

            /* Insert first disk */
            retro_disk_set_image_index(0);

            /* Trick frontend to return to index 0 after successful "append" does +1 */
            dc->replace = true;
        }
        /* Single append */
        else
        {
            char image_label[RETRO_PATH_MAX];
            image_label[0] = '\0';
            fill_short_pathname_representation(image_label, full_path_replace, sizeof(image_label));

            dc->files[index]  = strdup(full_path_replace);
            dc->labels[index] = get_label(full_path_replace);
            dc->types[index]  = dc_get_image_type(full_path_replace);
            dc->names[index]  = strdup(image_label);
        }
    }

    return true;
}

void dc_parse_list(dc_storage* dc, const char* list_file, bool is_vfl)
{
    /* Verify */
    if (dc == NULL)
        return;

    /* Reset */
    dc_reset(dc);

    if (list_file == NULL)
        return;

    FILE* fp = NULL;

    /* Try to open the file */
    if ((fp = fopen(list_file, "r")) == NULL)
    {
        log_cb(RETRO_LOG_ERROR, "Failed to open list file %s\n", list_file);
        return;
    }

    /* Read the lines while there is line to read and we have enough space */
    char buffer[2048];

    /* Enforce standard compatibility to avoid invalid vfl files on the loose */
    if (is_vfl)
    {
        /* Read and verify header */
        if (fgets(buffer, sizeof(buffer), fp) == NULL
            || strncmp(buffer, flip_file_header, strlen(flip_file_header)) != 0)
        {
            log_cb(RETRO_LOG_ERROR, "File %s is not a fliplist file\n", list_file);
            fclose(fp);
            return;
        }
    }

    /* Get the list base dir for resolving relative path */
    char* basedir = dirname_int(list_file);

    /* Label for the following file */
    char* label = NULL;

    /* Disk control interface 'name' for the following file */
    char* image_name = NULL;

    while ((dc->count <= DC_MAX_SIZE) && (fgets(buffer, sizeof(buffer), fp) != NULL))
    {
        char* string = trimwhitespace(buffer);

        if (string[0] == '\0')
            continue;

        /* Is it a m3u special key? (#COMMAND:) */
        if (!is_vfl && strstartswith(string, M3U_SPECIAL_COMMAND))
        {
            dc->command = strright(string, strlen(string) - strlen(M3U_SPECIAL_COMMAND));
        }
        /* Disk name / label (#LABEL:) */
        else if (!is_vfl && strstartswith(string, M3U_NONSTD_LABEL))
        {
            char* newlabel = trimwhitespace(buffer + strlen(M3U_NONSTD_LABEL));
            free(label);
            free(image_name);
            if (newlabel && newlabel[0])
            {
                label = strdup(newlabel);
                image_name = strdup(newlabel);
            }
            else
            {
                label = NULL;
                image_name = NULL;
            }
        }
        /* Disk name / label - EXTM3U standard compiant (#EXTINF) */
        else if (!is_vfl && strstartswith(string, M3U_EXTSTD_LABEL))
        {
            /* "Track" description should be following comma (https://en.wikipedia.org/wiki/M3U#Extended_M3U) */
            char* newlabel = strchr(buffer + strlen(M3U_EXTSTD_LABEL), ',');
            if (newlabel)
                newlabel = trimwhitespace(newlabel);
            free(label);
            free(image_name);
            if (newlabel && newlabel[0])
            {
                label = strdup(newlabel);
                image_name = strdup(newlabel);
            }
            else
            {
                label = NULL;
                image_name = NULL;
            }
        }
        /* VFL UNIT number (UNIT) */
        else if (is_vfl && strstartswith(string, VFL_UNIT_ENTRY))
        {
            int unit = strtol(string + strlen(VFL_UNIT_ENTRY), NULL, 10);
            /* VICE doesn't allow flip list for tape (unit 1),
             * but let's not split hairs */
            if (unit != 1 && !(unit >= 8 && unit <= 11))
            {
                /* Invalid unit number - just ignore the list */
                log_cb(RETRO_LOG_ERROR, "Invalid unit number %d in fliplist %s", unit, list_file);
                break;
            }
            if (dc->unit != 0 && dc->unit != unit && dc->count != 0)
            {
                /* VFL file could theoretically contain flip lists for multliple drives - read only the first one. */
                log_cb(RETRO_LOG_WARN, "Ignored entries for other unit(s) in fliplist %s", list_file);
                break;
            }
            dc->unit = unit;
        }
        /* Vice doesn't allow comments in vfl files - enforce the standard */
        else if (is_vfl || string[0] != COMMENT)
        {
            /* Search the file (absolute, relative to m3u) */
            char* filename;
            if ((filename = m3u_search_file(basedir, string)) != NULL)
            {
                /* If image name is missing, use filename without extension */
                if (!image_name)
                {
                    char tmp[512];
                    tmp[0] = '\0';

                    fill_short_pathname_representation(tmp, filename, sizeof(tmp));
                    image_name = strdup(tmp);
                }

                /* ZIP */
                if (strendswith(filename, "zip"))
                {
                    char full_path[RETRO_PATH_MAX] = {0};
                    snprintf(full_path, sizeof(full_path), "%s", filename);

                    char zip_basename[RETRO_PATH_MAX] = {0};
                    snprintf(zip_basename, sizeof(zip_basename), "%s", path_basename(full_path));
                    snprintf(zip_basename, sizeof(zip_basename), "%s", path_remove_extension(zip_basename));
                    snprintf(retro_temp_directory, sizeof(retro_temp_directory), "%s%s%s", retro_save_directory, FSDEV_DIR_SEP_STR, "TEMP");
                    char lastfile[RETRO_PATH_MAX] = {0};

                    path_mkdir(retro_temp_directory);
                    zip_uncompress(full_path, retro_temp_directory, lastfile);
                    snprintf(full_path, RETRO_PATH_MAX, "%s%s%s", retro_temp_directory, FSDEV_DIR_SEP_STR, lastfile);

                    /* Add the file to the struct */
                    dc_add_file(dc, full_path);
                }
                else
                {
                    /* Add the file to the struct */
                    dc_add_file_int(dc, filename, label ? label : get_label(filename), image_name);
                    label = NULL;
                    image_name = NULL;
                }
            }
            else
            {
                log_cb(RETRO_LOG_WARN, "File %s from list %s not found in dir %s\n", string, list_file, basedir);
                /* Throw away the label and image name */
                free(label);
                label = NULL;
                free(image_name);
                image_name = NULL;
            }
        }
    }

    /* If it's vfl, we have to reverse it */
    if (is_vfl)
    {
        int idx = 0;
        int ridx = dc->count - 1;
        while (idx < ridx)
        {
            char* tmp = dc->files[idx];
            dc->files[idx] = dc->files[ridx];
            dc->files[ridx] = tmp;
            tmp = dc->labels[idx];
            dc->labels[idx] = dc->labels[ridx];
            dc->labels[ridx] = tmp;
            tmp = dc->names[idx];
            dc->names[idx] = dc->names[ridx];
            dc->names[ridx] = tmp;
            ++idx; --ridx;
        }
    }

    free(basedir);
    free(label);
    free(image_name);

    /* Close the file */
    fclose(fp);

    /* M3U - Determine if tape or disk fliplist from first entry */
    if (dc->count != 0)
    {
        if (dc_get_image_type(dc->files[0]) == DC_IMAGE_TYPE_TAPE)
            dc->unit = 1;
        else if (dc_get_image_type(dc->files[0]) == DC_IMAGE_TYPE_FLOPPY)
            dc->unit = 8;
        else if (dc_get_image_type(dc->files[0]) == DC_IMAGE_TYPE_MEM)
            dc->unit = 0;
        else
            dc->unit = 8;

        if (runstate == RUNSTATE_RUNNING)
        {
            if (dc->unit == 1)
            {
                /* Detach & disable drive 8 */
                file_system_detach_disk(8);
                log_resources_set_int("Drive8Type", DRIVE_TYPE_NONE);
            }
            else if (dc->unit == 8)
            {
                /* Detach & disable tape, enable drive 8 */
                tape_deinstall();
                log_resources_set_int("Drive8Type", DRIVE_TYPE_1541);
            }
            else if (dc->unit == 0)
            {
                /* Detach & disable tape, detach & disable drive 8 */
                tape_deinstall();
                file_system_detach_disk(8);
                log_resources_set_int("Drive8Type", DRIVE_TYPE_NONE);
            }
        }
    }
}

void dc_parse_m3u(dc_storage* dc, const char* m3u_file)
{
    dc_parse_list(dc, m3u_file, false);
}

void dc_parse_vfl(dc_storage* dc, const char* vfl_file)
{
    dc_parse_list(dc, vfl_file, true);
}

void dc_free(dc_storage* dc)
{
    /* Clean the struct */
    dc_reset(dc);
    free(dc);
    dc = NULL;
    return;
}

enum dc_image_type dc_get_image_type(const char* filename)
{
	/* Missing file */
	if (!filename || (*filename == '\0'))
		return DC_IMAGE_TYPE_NONE;

	/* Floppy image */
	if (strendswith(filename, "d64") ||
	    strendswith(filename, "d71") ||
	    strendswith(filename, "d80") ||
	    strendswith(filename, "d81") ||
	    strendswith(filename, "d82") ||
	    strendswith(filename, "g64") ||
	    strendswith(filename, "x64") ||
	    strendswith(filename, "d6z") ||
	    strendswith(filename, "d7z") ||
	    strendswith(filename, "d8z") ||
	    strendswith(filename, "g6z") ||
	    strendswith(filename, "g4z") ||
	    strendswith(filename, "x6z"))
	   return DC_IMAGE_TYPE_FLOPPY;

	/* Tape image */
	if (strendswith(filename, "tap") ||
	    strendswith(filename, "t64"))
	   return DC_IMAGE_TYPE_TAPE;

	/* Memory image */
	if (strendswith(filename, "prg") ||
	    strendswith(filename, "p00") ||
	    strendswith(filename, "20")  ||
	    strendswith(filename, "40")  ||
	    strendswith(filename, "60")  ||
	    strendswith(filename, "a0")  ||
	    strendswith(filename, "b0")  ||
	    strendswith(filename, "crt") ||
	    strendswith(filename, "rom") ||
	    strendswith(filename, "bin"))
	   return DC_IMAGE_TYPE_MEM;

	/* Nibbler floppy image, requires conversion */
	if (strendswith(filename, "nib") ||
	    strendswith(filename, "nbz"))
	   return DC_IMAGE_TYPE_NIBBLER;

	/* Fallback */
	return DC_IMAGE_TYPE_UNKNOWN;
}
