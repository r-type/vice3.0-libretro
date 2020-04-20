#include "libretro-core.h"
#include "archdep.h"

#include "deps/libz/zlib.h"
#include "deps/libz/unzip.h"
#include "file/file_path.h"
void zip_uncompress(char *in, char *out, char *lastfile)
{
    unzFile uf = NULL;
    unz_file_info file_info;
    uf = unzOpen(in);

    uLong i;
    unz_global_info gi;
    int err;
    err = unzGetGlobalInfo (uf, &gi);

    const char* password = NULL;
    int size_buf = 8192;

    for (i = 0; i < gi.number_entry; i++)
    {
        char filename_inzip[256];
        char filename_withpath[512];
        filename_inzip[0] = '\0';
        filename_withpath[0] = '\0';
        char* filename_withoutpath;
        char* p;
        unz_file_info file_info;
        FILE *fout = NULL;
        void* buf;

        buf = (void*)malloc(size_buf);
        if (buf == NULL)
        {
            fprintf(stderr, "Unzip: Error allocating memory\n");
            return;
        }

        err = unzGetCurrentFileInfo(uf, &file_info, filename_inzip, sizeof(filename_inzip), NULL, 0, NULL, 0);
        snprintf(filename_withpath, sizeof(filename_withpath), "%s%s%s", out, FSDEV_DIR_SEP_STR, filename_inzip);
        if ((dc_get_image_type(filename_inzip) == DC_IMAGE_TYPE_FLOPPY || dc_get_image_type(filename_inzip) == DC_IMAGE_TYPE_TAPE) && lastfile != NULL)
            snprintf(lastfile, RETRO_PATH_MAX, "%s", filename_inzip);
        p = filename_withoutpath = filename_inzip;
        while ((*p) != '\0')
        {
            if (((*p) == '/') || ((*p) == '\\'))
                filename_withoutpath = p+1;
            p++;
        }

        if ((*filename_withoutpath) == '\0')
        {
            fprintf(stdout, "Unzip mkdir:   %s\n", filename_withpath);
            path_mkdir(filename_withpath);
        }
        else
        {
            const char* write_filename;
            int skip = 0;

            write_filename = filename_withpath;

            err = unzOpenCurrentFilePassword(uf, password);
            if (err != UNZ_OK)
            {
                fprintf(stderr, "Unzip: Error %d with zipfile in unzOpenCurrentFilePassword: %s\n", err, write_filename);
            }

            if ((skip == 0) && (err == UNZ_OK))
            {
                fout = fopen(write_filename, "wb");
                if (fout == NULL)
                {
                    fprintf(stderr, "Unzip: Error opening %s\n", write_filename);
                }
            }

            if (fout != NULL)
            {
                fprintf(stdout, "Unzip extract: %s\n", write_filename);

                do
                {
                    err = unzReadCurrentFile(uf, buf, size_buf);
                    if (err < 0)
                    {
                        fprintf(stderr, "Unzip: Error %d with zipfile in unzReadCurrentFile\n", err);
                        break;
                    }
                    if (err > 0)
                    {
                        if (!fwrite(buf, err, 1, fout))
                        {
                            fprintf(stderr, "Unzip: Error in writing extracted file\n");
                            err = UNZ_ERRNO;
                            break;
                        }
                    }
                }
                while (err > 0);
                if (fout)
                    fclose(fout);
            }

            if (err == UNZ_OK)
            {
                err = unzCloseCurrentFile(uf);
                if (err != UNZ_OK)
                {
                    fprintf(stderr, "Unzip: Error %d with zipfile in unzCloseCurrentFile\n", err);
                }
            }
            else
                unzCloseCurrentFile(uf);
        }

        free(buf);

        if ((i+1) < gi.number_entry)
        {
            err = unzGoToNextFile(uf);
            if (err != UNZ_OK)
            {
                fprintf(stderr, "Unzip: Error %d with zipfile in unzGoToNextFile\n", err);
                break;
            }
        }
    }

    if (uf)
    {
        unzCloseCurrentFile(uf);
        unzClose(uf);
        uf = NULL;
    }
}

void remove_recurse(const char *path)
{
   struct dirent *dirp;
   char filename[RETRO_PATH_MAX];
   DIR *dir = opendir(path);
   if (dir == NULL)
      return;

   while ((dirp = readdir(dir)) != NULL)
   {
      if (dirp->d_name[0] == '.')
         continue;

      sprintf(filename, "%s%s%s", path, FSDEV_DIR_SEP_STR, dirp->d_name);
      fprintf(stdout, "Unzip clean: %s\n", filename);

      if (path_is_directory(filename))
         remove_recurse(filename);
      else
         remove(filename);
   }

   closedir(dir);
#ifdef VITA
   sceIoRmdir(path);
#else
   rmdir(path);
#endif
}
