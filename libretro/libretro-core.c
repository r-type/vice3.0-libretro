#include "libretro.h"
#include "libretro-core.h"
#include "encodings/utf.h"

#include "archdep.h"
#include "mem.h"
#include "machine.h"
#include "snapshot.h"
#include "autostart.h"
#include "drive.h"
#include "tape.h"
#include "vdrive.h"
#include "diskimage.h"
#include "vdrive-internal.h"
#include "charset.h"
#include "attach.h"
#include "interrupt.h"
#include "datasette.h"
#include "cartridge.h"
#include "initcmdline.h"
#include "vsync.h"
#include "log.h"
#include "keyboard.h"
#include "resources.h"
#include "sid.h"
#if !defined(__XCBM5x0__)
#include "userport_joystick.h"
#endif

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

// Accurate tick for statusbar FPS counter
long retro_now = 0;
// Main CPU loop
int cpuloop = 1;

// VKBD 
extern int SHOWKEY;
unsigned int opt_vkbd_theme = 0;
unsigned int opt_vkbd_alpha = 204;
unsigned int vkbd_alpha = 204;

// Core vars
extern char core_key_state[512];
extern char core_old_key_state[512];
static bool noautostart = false;
static char* autostartString = NULL;
static char full_path[RETRO_PATH_MAX] = {0};
char RETRO_DIR[RETRO_PATH_MAX];
static char* core_options_legacy_strings = NULL;

static snapshot_stream_t* snapshot_stream = NULL;
static int load_trap_happened = 0;
static int save_trap_happened = 0;

int mapper_keys[36] = {0};
unsigned int vice_devices[5];
unsigned int opt_mapping_options_display;
unsigned int opt_video_options_display;
unsigned int opt_audio_options_display;
unsigned int retro_region;
static double retro_refresh;
static unsigned int prev_sound_sample_rate = 0;

extern void retro_poll_event();
extern int retro_ui_finalized;
static int prev_ui_finalized = 0;

extern uint8_t mem_ram[];
extern int g_mem_ram_size;

// Core geometry
int retroXS = 0;
int retroYS = 0;
int retroXS_offset = 0;
int retroYS_offset = 0;
int defaultW = WINDOW_WIDTH;
int defaultH = WINDOW_HEIGHT;
int retroW = WINDOW_WIDTH;
int retroH = WINDOW_HEIGHT;
int lastW = 0;
int lastH = 0;

unsigned int pix_bytes = 2;
static bool pix_bytes_initialized = false;
unsigned short int retro_bmp[RETRO_BMP_SIZE];

// Core options
struct libretro_core_options core_opt;

#if defined(__XVIC__)
int vic20mem_forced = -1;
#endif

unsigned int zoom_mode_id = 0;
int zoom_mode_id_prev = -1;
unsigned int opt_zoom_mode_id = 0;
unsigned int zoom_mode_crop_id = 0;
unsigned int zoomed_width;
unsigned int zoomed_height;
unsigned int zoomed_XS_offset;
unsigned int zoomed_YS_offset;
static unsigned int opt_aspect_ratio = 0;
static unsigned int manual_crop_top = 0;
static unsigned int manual_crop_bottom = 0;
static unsigned int manual_crop_left = 0;
static unsigned int manual_crop_right = 0;

static unsigned int request_reload_restart = 0;
static bool request_update_work_disk = false;
static int request_model_set = -1;
static int request_model_prev = -1;
static unsigned int opt_model_auto = 1;
unsigned int opt_autoloadwarp = 0;
unsigned int opt_read_vicerc = 0;
static unsigned int opt_read_vicerc_prev = 0;
static unsigned int opt_work_disk_type = 0;
static unsigned int opt_work_disk_unit = 8;
#if defined(__X64__) || defined(__X64SC__) || defined(__X128__) || defined(__XSCPU64__)
static unsigned int opt_jiffydos_allow = 1;
unsigned int opt_jiffydos = 0;
static unsigned int opt_jiffydos_prev = 0;
#endif
static unsigned int sound_volume_counter = 3;
unsigned int opt_audio_leak_volume = 0;
unsigned int opt_statusbar = 0;
unsigned int opt_reset_type = 0;
unsigned int opt_keyrah_keypad = 0;
unsigned int opt_keyboard_keymap = KBD_INDEX_POS;
unsigned int opt_keyboard_passthrough = 0;
unsigned int opt_retropad_options = 0;
unsigned int opt_joyport_type = 0;
unsigned int opt_dpadmouse_speed = 6;
unsigned int opt_mouse_speed = 100;
unsigned int opt_analogmouse = 0;
unsigned int opt_analogmouse_deadzone = 15;
float opt_analogmouse_speed = 1.0;

extern unsigned int datasette_hotkeys;
extern unsigned int cur_port;
extern int cur_port_locked;

extern int turbo_fire_button;
extern unsigned int turbo_pulse;

// VICE includes
#if defined(__X64__) || defined(__X64SC__)
#include "c64.h"
#include "c64mem.h"
#include "c64model.h"
#elif defined(__X128__)
#include "c128.h"
#include "c128mem.h"
#include "c128model.h"
#elif defined(__XCBM2__) || defined(__XCBM5x0__)
#include "cbm2.h"
#include "cbm2model.h"
void cartridge_trigger_freeze(void) {}
#elif defined(__XPET__)
#include "pet.h"
#include "petmodel.h"
void cartridge_detach_image(int type) {}
void cartridge_trigger_freeze(void) {}
#elif defined(__XPLUS4__)
#include "plus4.h"
#include "plus4model.h"
void cartridge_trigger_freeze(void) {}
#elif defined(__XSCPU64__)
#include "scpu64.h"
#include "c64mem.h"
#include "c64model.h"
#elif defined(__XVIC__)
#include "vic20.h"
#include "vic20mem.h"
#include "vic20model.h"
#include "vic20cart.h"
void cartridge_trigger_freeze(void) {}
#endif

char retro_save_directory[RETRO_PATH_MAX] = {0};
char retro_temp_directory[RETRO_PATH_MAX] = {0};
char retro_system_directory[RETRO_PATH_MAX] = {0};
char retro_content_directory[RETRO_PATH_MAX] = {0};
char retro_system_data_directory[RETRO_PATH_MAX] = {0};

retro_input_state_t input_state_cb;
retro_input_poll_t input_poll_cb;
retro_log_printf_t log_cb;
static retro_video_refresh_t video_cb;
static retro_audio_sample_t audio_cb;
static retro_audio_sample_batch_t audio_batch_cb;
static retro_environment_t environ_cb;

static dc_storage* dc;

int runstate = RUNSTATE_FIRST_START; /* used to detect whether we are just starting the core from scratch */
/* runstate = RUNSTATE_FIRST_START: first time retro_run() is called after loading and starting core */
/* runstate = RUNSTATE_LOADED_CONTENT: load content was selected while core is running, so do an autostart_reset() */
/* runstate = RUNSTATE_RUNNING: core is running normally */

/* Display disk name and label instead of "Changing disk in tray"- maybe make it configurable */
bool display_disk_name = true;
/* See which looks best in most cases and tweak (or make configurable) */
int disk_label_mode = DISK_LABEL_MODE_ASCII_OR_CAMELCASE;

static char* x_strdup(const char* str)
{
   return str ? strdup(str) : NULL;
}

void retro_set_input_state(retro_input_state_t cb)
{
   input_state_cb = cb;
}

void retro_set_input_poll(retro_input_poll_t cb)
{
   input_poll_cb = cb;
}

static char CMDFILE[512];

int loadcmdfile(const char *argv)
{
   int res=0;

   FILE *fp = fopen(argv,"r");

   CMDFILE[0] = '\0';
   if (fp != NULL)
   {
      if (fgets(CMDFILE , 512 , fp) != NULL)
         res=1;
      fclose (fp);
   }

   return res;
}

#include <ctype.h>

// Args for experimental_cmdline
static char ARGUV[64][1024];
static unsigned char ARGUC=0;

// Args for Core
static char XARGV[64][1024];
static const char* xargv_cmd[64];
int PARAMCOUNT=0;

// Display message on next retro_run
static char queued_msg[1024];
static int show_queued_msg = 0;

extern int skel_main(int argc, char *argv[]);

static void log_disk_in_tray(bool display);
extern void display_current_image(const char *image, bool inserted);

static void Add_Option(const char* option)
{
   sprintf(XARGV[PARAMCOUNT++],"%s",option);
}

static void parse_cmdline(const char *argv)
{
   char *p,*p2,*start_of_word;
   int c,c2;
   static char buffer[512*4];
   enum states { DULL, IN_WORD, IN_STRING } state = DULL;

   ARGUC = 0;

   strcpy(buffer,argv);
   strcat(buffer," \0");

   for (p = buffer; *p != '\0'; p++)
   {
      c = (unsigned char) *p; /* convert to unsigned char for is* functions */
      switch (state)
      {
         case DULL: /* not in a word, not in a double quoted string */
            if (isspace(c)) /* still not in a word, so ignore this char */
               continue;
            /* not a space -- if it's a double quote we go to IN_STRING, else to IN_WORD */
            if (c == '"')
            {
               state = IN_STRING;
               start_of_word = p + 1; /* word starts at *next* char, not this one */
               continue;
            }
            state = IN_WORD;
            start_of_word = p; /* word starts here */
            continue;
         case IN_STRING:
            /* we're in a double quoted string, so keep going until we hit a close " */
            if (c == '"')
            {
               /* word goes from start_of_word to p-1 */
               //... do something with the word ...
               for (c2 = 0,p2 = start_of_word; p2 < p; p2++, c2++)
                  ARGUV[ARGUC][c2] = (unsigned char) *p2;
               /* terminate word */
               ARGUV[ARGUC][c2] = '\0';
               ARGUC++;

               state = DULL; /* back to "not in word, not in string" state */
            }
            continue; /* either still IN_STRING or we handled the end above */
         case IN_WORD:
            /* we're in a word, so keep going until we get to a space */
            if (isspace(c))
            {
               /* word goes from start_of_word to p-1 */
               //... do something with the word ...
               for (c2 = 0,p2 = start_of_word; p2 <p; p2++,c2++)
                  ARGUV[ARGUC][c2] = (unsigned char) *p2;
               /* terminate word */
               ARGUV[ARGUC][c2] = '\0';
               ARGUC++;

               state = DULL; /* back to "not in word, not in string" state */
            }
            continue; /* either still IN_WORD or we handled the end above */
      }
   }
}

static int check_joystick_control(const char* filename)
{
    int port = 0;
    if (filename != NULL)
    {
        if (strcasestr(filename, "_j1.") || strcasestr(filename, "(j1)."))
            port = 1;
        else if (strcasestr(filename, "_j2.") || strcasestr(filename, "(j2)."))
            port = 2;
    }
    return port;
}

static int get_image_unit()
{
    int unit = dc->unit;
    if (dc->index < dc->count)
    {
        if (dc_get_image_type(dc->files[dc->index]) == DC_IMAGE_TYPE_TAPE)
            dc->unit = 1;
        else if (dc_get_image_type(dc->files[dc->index]) == DC_IMAGE_TYPE_FLOPPY)
            dc->unit = 8;
        else if (dc_get_image_type(dc->files[dc->index]) == DC_IMAGE_TYPE_MEM)
            dc->unit = 0;
        else
            dc->unit = 8;
    }
    else
        unit = 8;

    return unit;
}

// If we display the message now, it wold be immediatelly overwritten
// by "changed disk in drive", so queue it to display on next retro_run.
// The side effect is that if disk is changed from menu, the message will be displayed
// only after emulation is unpaused.
static void log_disk_in_tray(bool display)
{
    if (dc->index < dc->count)
    {
        int unit = get_image_unit();
        size_t pos = 0;
        const char* label;
        // Build message do display
        if (unit == 1)
            snprintf(queued_msg, sizeof(queued_msg), "Tape: ");
        else if (unit == 8)
            snprintf(queued_msg, sizeof(queued_msg), "Drive %d: ", unit);
        else
            snprintf(queued_msg, sizeof(queued_msg), "Cart: ");
        pos = strlen(queued_msg);
        snprintf(queued_msg + pos, sizeof(queued_msg) - pos, "(%d/%d) %s", dc->index + 1, dc->count, path_basename(dc->files[dc->index]));
        pos += strlen(queued_msg + pos);
        label = dc->labels[dc->index];
        if (label && label[0])
        {
            snprintf(queued_msg + pos, sizeof(queued_msg) - pos, " (%s)", label);
        }
        log_cb(RETRO_LOG_INFO, "%s\n", queued_msg);
        if (display)
            show_queued_msg = 150;
    }
}

static int process_cmdline(const char* argv)
{
    int i = 0;
    bool is_fliplist = false;
    int joystick_control = 0;

#if defined(__XPLUS4__)
    // Do not reset noautostart if already set, PLUS/4 has issues with starting carts via autostart (?!)
    noautostart = (noautostart) ? noautostart : false;
#else
    noautostart = false;
#endif
    PARAMCOUNT = 0;
    dc_reset(dc);

    cur_port_locked = 0;
    free(autostartString);
    autostartString = NULL;

    // Load command line arguments from cmd file
    if (strendswith(argv, ".cmd"))
    {
        if (loadcmdfile(argv))
        {
            argv = trimwhitespace(CMDFILE);
            log_cb(RETRO_LOG_INFO, "Starting game from command line: %s\n", argv);
            core_opt.Model = 99; // set model to unknown for custom settings - prevents overriding of command line options
        }
        else
        {
            log_cb(RETRO_LOG_ERROR, "Failed to load command line from %s\n", argv);
            argv = CMDFILE;
        }
    }
    parse_cmdline(argv);

    // Core command line is now parsed to ARGUV, ARGUC.
    // Build command file for VICE in XARGV, PARAMCOUNT.

    bool single_image = strcmp(ARGUV[0], CORE_NAME) != 0;

    // If first command line argument is "x64", it's and extended command line
    // otherwise it's just image filename
    if (single_image)
    {
        Add_Option(CORE_NAME);

        // Ignore parsed arguments, read filename directly from argv

        // Check original filename for joystick control,
        // not the first name from M3U or VFL
        joystick_control = check_joystick_control(argv);
        if (joystick_control)
        {
            cur_port = joystick_control;
            cur_port_locked = 1;
        }

        // "Browsed" file in ZIP
        char browsed_file[RETRO_PATH_MAX] = {0};
        if (strstr(argv, ".zip#"))
        {
            char *token = strtok((char*)argv, "#");
            while (token != NULL)
            {
                snprintf(browsed_file, sizeof(browsed_file), "%s", token);
                token = strtok(NULL, "#");
            }
        }
        snprintf(full_path, sizeof(full_path), "%s", argv);

        // ZIP + NIB vars, use the same temp directory for single NIBs
        char zip_basename[RETRO_PATH_MAX] = {0};
        snprintf(zip_basename, sizeof(zip_basename), "%s", path_basename(full_path));
        snprintf(zip_basename, sizeof(zip_basename), "%s", path_remove_extension(zip_basename));
        snprintf(retro_temp_directory, sizeof(retro_temp_directory), "%s%s%s", retro_save_directory, FSDEV_DIR_SEP_STR, "ZIP");
        char zip_path[RETRO_PATH_MAX] = {0};
        snprintf(zip_path, sizeof(zip_path), "%s%s%s", retro_temp_directory, FSDEV_DIR_SEP_STR, zip_basename);

        char nib_input[RETRO_PATH_MAX] = {0};
        char nib_output[RETRO_PATH_MAX] = {0};

        // NIB convert to G64
        if (dc_get_image_type(argv) == DC_IMAGE_TYPE_NIBBLER)
        {
            snprintf(nib_input, sizeof(nib_input), "%s", argv);
            snprintf(nib_output, sizeof(nib_output), "%s%s%s.g64", zip_path, FSDEV_DIR_SEP_STR, zip_basename);
            path_mkdir(zip_path);
            nib_convert(nib_input, nib_output);
            argv = nib_output;
        }

        // ZIP
        if (strendswith(argv, ".zip"))
        {
            path_mkdir(zip_path);
            zip_uncompress(full_path, zip_path, NULL);

            // Default to directory mode
            int zip_mode = 0;
            snprintf(full_path, sizeof(full_path), "%s", zip_path);

            FILE *zip_m3u;
            char zip_m3u_list[20][RETRO_PATH_MAX] = {0};
            char zip_m3u_path[RETRO_PATH_MAX] = {0};
            snprintf(zip_m3u_path, sizeof(zip_m3u_path), "%s%s%s.m3u", zip_path, FSDEV_DIR_SEP_STR, zip_basename);
            int zip_m3u_num = 0;

            DIR *zip_dir;
            struct dirent *zip_dirp;

            // Convert all NIBs to G64
            zip_dir = opendir(zip_path);
            while ((zip_dirp = readdir(zip_dir)) != NULL)
            {
                if (dc_get_image_type(zip_dirp->d_name) == DC_IMAGE_TYPE_NIBBLER)
                {
                    snprintf(nib_input, sizeof(nib_input), "%s%s%s", zip_path, FSDEV_DIR_SEP_STR, zip_dirp->d_name);
                    snprintf(nib_output, sizeof(nib_output), "%s%s%s.g64", zip_path, FSDEV_DIR_SEP_STR, path_remove_extension(zip_dirp->d_name));
                    nib_convert(nib_input, nib_output);
                }
            }
            closedir(zip_dir);

            zip_dir = opendir(zip_path);
            while ((zip_dirp = readdir(zip_dir)) != NULL)
            {
                if (zip_dirp->d_name[0] == '.' || strendswith(zip_dirp->d_name, ".m3u") || zip_mode > 1 || browsed_file[0] != '\0')
                    continue;

                // Multi file mode, generate playlist
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
                case 0: // Extracted path
                    if (browsed_file[0] != '\0')
                    {
                        if (dc_get_image_type(browsed_file) == DC_IMAGE_TYPE_NIBBLER)
                            snprintf(full_path, sizeof(full_path), "%s%s%s.g64", zip_path, FSDEV_DIR_SEP_STR, path_remove_extension(browsed_file));
                        else
                            snprintf(full_path, sizeof(full_path), "%s%s%s", zip_path, FSDEV_DIR_SEP_STR, browsed_file);
                    }
                    break;
                case 1: // Generated playlist
                    zip_m3u = fopen(zip_m3u_path, "w");
                    qsort(zip_m3u_list, zip_m3u_num, RETRO_PATH_MAX, qstrcmp);
                    for (int l = 0; l < zip_m3u_num; l++)
                        fprintf(zip_m3u, "%s\n", zip_m3u_list[l]);
                    fclose(zip_m3u);
                    snprintf(full_path, sizeof(full_path), "%s", zip_m3u_path);
                    break;
            }

            argv = full_path;
        }

#if defined(__X64__) || defined(__X64SC__) || defined(__X128__) || defined(__XSCPU64__)
        // Do not allow JiffyDOS with non-floppies
        if (dc_get_image_type(argv) == DC_IMAGE_TYPE_TAPE
         || dc_get_image_type(argv) == DC_IMAGE_TYPE_MEM)
            opt_jiffydos_allow = 0;
        else
            opt_jiffydos_allow = 1;
#endif

#if defined(__XVIC__)
        if (strendswith(argv, ".20"))
            Add_Option("-cart2");
        else if (strendswith(argv, ".40"))
            Add_Option("-cart4");
        else if (strendswith(argv, ".60"))
            Add_Option("-cart6");
        else if (strendswith(argv, ".a0"))
            Add_Option("-cartA");
        else if (strendswith(argv, ".b0"))
            Add_Option("-cartB");

        char vic20buf1[6]   = "\0";
        char vic20buf2[6]   = "\0";
        int vic20mem        = 0;
        int vic20mems[5]    = {0, 3, 8, 16, 24};

        for (int i = 0; i < 5; i++)
        {
            vic20mem = vic20mems[i];
            snprintf(vic20buf1, 6, "%c%d%c%c", '(', vic20mem, 'k', ')');
            snprintf(vic20buf2, 6, "%c%d%c%c", FSDEV_DIR_SEP_CHR, vic20mem, 'k', FSDEV_DIR_SEP_CHR);
            if (strcasestr(argv, vic20buf1))
            {
                vic20mem_forced = i;
                log_cb(RETRO_LOG_INFO, "VIC-20 memory expansion force found in filename '%s': %dKB\n", argv, vic20mem);
                break;
            }
            else if (strcasestr(argv, vic20buf2))
            {
                vic20mem_forced = i;
                log_cb(RETRO_LOG_INFO, "VIC-20 memory expansion force found in path '%s': %dKB\n", argv, vic20mem);
                break;
            }
        }
#elif defined(__XPLUS4__)
        if (strendswith(argv, ".crt") || strendswith(argv, ".bin"))
            Add_Option("-cart");
#endif

        if (strendswith(argv, ".m3u"))
        {
            // Parse the m3u file
            dc_parse_m3u(dc, argv);
            is_fliplist = true;
        }
        else if (strendswith(argv, ".vfl"))
        {
            // Parse the vfl file
            dc_parse_vfl(dc, argv);
            is_fliplist = true;
        }

        if (!is_fliplist)
        {
            // Add image name as autostart parameter
            Add_Option(argv);
        }
        else
        {
            // Some debugging
            log_cb(RETRO_LOG_INFO, "M3U/VFL parsed, %d file(s) found\n", dc->count);

            if (!dc->command)
            {
                // Add first disk from list as autostart parameter
                if (dc->count != 0)
                    Add_Option(dc->files[0]);
            }
            else
            {
                // Re-parse command line from M3U #COMMAND:
                log_cb(RETRO_LOG_INFO, "Starting game from command line: %s\n", dc->command);
                core_opt.Model = 99; // set model to unknown for custom settings - prevents overriding of command line options
                parse_cmdline(dc->command);
                // Reset parameters list for Vice
                PARAMCOUNT = 0;
                single_image = false;
            }
        }
    }

    // It might be single_image initially, but changed by M3U file #COMMAND line
    if (!single_image)
    {
        if (ARGUC == 0 || strcmp(ARGUV[0], CORE_NAME) != 0)
        {
            // Command doesn't start with core name, so add it first
            Add_Option(CORE_NAME);
        }

        bool is_flipname_param = false;
        // Scan vice arguments for special processing
        for (i = 0; i < ARGUC; i++)
        {
            const char* arg = ARGUV[i];

            // Previous arg was '-flipname'
            if (is_flipname_param)
            {
                is_flipname_param = false;
                // Parse the vfl file, don't pass to vice
                dc_parse_vfl(dc, arg);
                // Don't pass -flipname argument to vice - it has no use of it
                // and we won't have to take care of cleaning it up
                is_fliplist = true;
            }
            else if (strcmp(arg, "-j1") == 0)
            {
                cur_port = 1;
                cur_port_locked = 1;
            }
            else if (strcmp(arg, "-j2") == 0)
            {
                cur_port = 2;
                cur_port_locked = 1;
            }
            else if (strendswith(arg, ".m3u"))
            {
                // Parse the m3u file, don't pass to vice
                dc_parse_m3u(dc, arg);
                is_fliplist = true;
            }
            else if (strcmp(arg, "-flipname") == 0)
            {
                // Set flag for next arg
                is_flipname_param = true;
            }
            else if (strcmp(arg, "-noautostart") == 0)
            {
                // User ask to not automatically start image in drive
                noautostart = true;
            }
            else
            {
                Add_Option(arg);
            }
        }

        if (is_fliplist)
        {
            // Some debugging
            log_cb(RETRO_LOG_INFO, "M3U/VFL parsed, %d file(s) found\n", dc->count);
        }
    }

    return 0;
}

static void autodetect_drivetype(int unit)
{
    int drive_type;
    char drive_type_resource_var[20] = {0};
    snprintf(drive_type_resource_var, sizeof(drive_type_resource_var), "Drive%dType", unit);
    resources_get_int(drive_type_resource_var, &drive_type);
    const char* attached_image = NULL;
    attached_image = file_system_get_disk_name(unit);

    // Autodetect drive type
    vdrive_t *vdrive;
    struct disk_image_s *diskimg;

    vdrive = file_system_get_vdrive(unit);
    if (vdrive == NULL)
        log_cb(RETRO_LOG_ERROR, "Failed to get vdrive reference for unit %d.\n", unit);
    else
    {
        diskimg = vdrive->image;

        // G64 will set a nonexistent drivetype, therefore force 1541
        if (diskimg != NULL && diskimg->type == DISK_IMAGE_TYPE_G64)
            diskimg->type = 1541;

        // G71 will set a nonexistent drivetype, therefore force 1571
        if (diskimg != NULL && diskimg->type == DISK_IMAGE_TYPE_G71)
            diskimg->type = 1571;

        if (diskimg == NULL)
            log_cb(RETRO_LOG_ERROR, "Failed to get disk image for unit %d.\n", unit);
        else if (diskimg->type != drive_type)
        {
            log_cb(RETRO_LOG_INFO, "Autodetected image type %u.\n", diskimg->type);
            if (log_resources_set_int(drive_type_resource_var, diskimg->type) < 0)
                log_cb(RETRO_LOG_ERROR, "Failed to set drive type.\n");

            // Change from 1581 to 1541 will not detect disk properly without reattaching (?!)
            file_system_attach_disk(unit, attached_image);

            // Don't bother with drive sound muting when autoloadwarp is on
            if (opt_autoloadwarp)
               return;
            // Drive motor sound keeps on playing if the drive type is changed while the motor is running
            // Also happens when toggling TDE
            vdrive = file_system_get_vdrive(unit);
            if (vdrive != NULL)
               diskimg = vdrive->image;
            switch (diskimg->type)
            {
                case 1541:
                case 1571:
                    resources_set_int("DriveSoundEmulationVolume", core_opt.DriveSoundEmulation);
                    break;
                case 1581:
                default:
                    resources_set_int("DriveSoundEmulationVolume", 0);
                    break;
            }
        }
    }
}

void update_work_disk()
{
    request_update_work_disk = false;
    const char* attached_image = NULL;

    // Skip if device unit collides with autostart
    if (!string_is_empty(full_path) && opt_work_disk_unit == 8)
        opt_work_disk_type = 0;
    if (opt_work_disk_type)
    {
        // Path vars
        char opt_work_disk_filename[RETRO_PATH_MAX] = {0};
        char opt_work_disk_filepath[RETRO_PATH_MAX] = {0};
        char opt_work_disk_extension[4] = {0};
        switch (opt_work_disk_type)
        {
            default:
            case DISK_IMAGE_TYPE_D64:
                snprintf(opt_work_disk_extension, sizeof(opt_work_disk_extension), "%s", "d64");
                break;
            case DISK_IMAGE_TYPE_D71:
                snprintf(opt_work_disk_extension, sizeof(opt_work_disk_extension), "%s", "d71");
                break;
            case DISK_IMAGE_TYPE_D81:
                snprintf(opt_work_disk_extension, sizeof(opt_work_disk_extension), "%s", "d81");
                break;
        }
        snprintf(opt_work_disk_filename, sizeof(opt_work_disk_filename), "vice_work.%s", opt_work_disk_extension);
        path_join((char*)&opt_work_disk_filepath, retro_save_directory, opt_work_disk_filename);

        // Create disk
        if (!file_exists(opt_work_disk_filepath))
        {
            // Label format
            char format_name[28];
            snprintf(format_name, sizeof(format_name), "%s-%s", "work", opt_work_disk_extension);
            charset_petconvstring((uint8_t *)format_name, 0);

            if (vdrive_internal_create_format_disk_image(opt_work_disk_filepath, format_name, opt_work_disk_type))
                log_cb(RETRO_LOG_INFO, "Work disk creation failed: '%s'\n", opt_work_disk_filepath);
            else
                log_cb(RETRO_LOG_INFO, "Work disk created: '%s'\n", opt_work_disk_filepath);
        }

        // Attach disk
        if (file_exists(opt_work_disk_filepath))
        {
            // Detach previous disks
            if ((attached_image = file_system_get_disk_name(8)) != NULL)
                file_system_detach_disk(8);

            if ((attached_image = file_system_get_disk_name(9)) != NULL)
            {
                file_system_detach_disk(9);
                log_resources_set_int("Drive9Type", DRIVE_TYPE_NONE);
            }

            if (opt_work_disk_unit == 9)
                log_resources_set_int("Drive9Type", opt_work_disk_type);
            file_system_attach_disk(opt_work_disk_unit, opt_work_disk_filepath);
            autodetect_drivetype(opt_work_disk_unit);
            log_cb(RETRO_LOG_INFO, "Work disk '%s' attached in drive #%d\n", opt_work_disk_filepath, opt_work_disk_unit);
            display_current_image(opt_work_disk_filename, true);
        }
    }
    else
    {
        // Detach work disk if disabled while running
        if ((attached_image = file_system_get_disk_name(8)) != NULL && strstr(attached_image, "vice_work"))
        {
            if (full_path == NULL || (full_path != NULL && !strstr(full_path, "vice_work")))
            {
                log_cb(RETRO_LOG_INFO, "Work disk '%s' detached from drive #%d\n", attached_image, 8);
                file_system_detach_disk(8);
                log_resources_set_int("Drive8Type", DRIVE_TYPE_1541);
                display_current_image(attached_image, false);
            }
        }

        if ((attached_image = file_system_get_disk_name(9)) != NULL && strstr(attached_image, "vice_work"))
        {
            log_cb(RETRO_LOG_INFO, "Work disk '%s' detached from drive #%d\n", attached_image, 9);
            file_system_detach_disk(9);
            log_resources_set_int("Drive9Type", DRIVE_TYPE_NONE);
        }
    }
}

// Update autostart image from vice and add disk in drive to fliplist
void update_from_vice()
{
    const char* attachedImage = NULL;

    // Get autostart string from vice, handle carts differently
    if (dc->unit == 0 && autostartString != NULL)
    {
        autostartString = NULL;
        attachedImage = dc->files[dc->index];
        // Disable AutostartWarp & WarpMode, otherwise warp gets stuck with PRGs in M3Us
        resources_set_int("AutostartWarp", 0);
        resources_set_int("WarpMode", 0);
    }
    else
    {
        free(autostartString);
        autostartString = x_strdup(cmdline_get_autostart_string());
    }

    if (autostartString)
        log_cb(RETRO_LOG_INFO, "Image for autostart: %s\n", autostartString);
    else
        log_cb(RETRO_LOG_INFO, "No image for autostart\n");

#if defined(__X64__) || defined(__X64SC__) || defined(__XSCPU64__)
    // Automatic model request
    if (opt_model_auto && autostartString)
    {
        if (strstr(autostartString, "NTSC") != NULL || strstr(autostartString, "(USA)") != NULL)
        {
            fprintf(stdout, "[libretro-vice]: Found 'NTSC' or '(USA)' in: '%s'\n", autostartString);

            request_model_set = C64MODEL_C64_NTSC;
            if (core_opt.Model == C64MODEL_C64C_PAL)
                request_model_set = C64MODEL_C64C_NTSC;
        }

        if (strstr(autostartString, "PAL") != NULL || strstr(autostartString, "(Europe)") != NULL)
        {
            fprintf(stdout, "[libretro-vice]: Found 'PAL' or '(Europe)' in: '%s'\n", autostartString);

            request_model_set = C64MODEL_C64_PAL;
            if (core_opt.Model == C64MODEL_C64C_NTSC)
                request_model_set = C64MODEL_C64C_PAL;
        }
    }
    else
        request_model_set = -1;
#endif

    // If flip list is empty, get current tape or floppy image name and add to the list
    if (dc->count == 0)
    {
        if ((attachedImage = tape_get_file_name()) != NULL)
        {
            dc->unit = 1;
            dc_add_file(dc, attachedImage);
        }
        else
        {
#if 0
            int unit;
            for (unit = 8; unit <= 11; ++unit)
            {
                if ((attachedImage = file_system_get_disk_name(unit)) != NULL)
                {
                    dc->unit = unit;
                    dc_add_file(dc, attachedImage);
                    break;
                }
            }
#else
            // Only add images to the list from device 8, otherwise leads to confusion when other devices have disks,
            // because Disk Control operates only on device 8 for now.
            int unit = 8;
            if ((attachedImage = file_system_get_disk_name(unit)) != NULL)
            {
                dc->unit = unit;
                dc_add_file(dc, attachedImage);
            }
#endif
        }
    }

#if defined(__X64__) || defined(__X64SC__) || defined(__X128__) || defined(__XSCPU64__)
    // Disable JiffyDOS with tapes and carts
    if (opt_jiffydos && dc->unit <= 1 && dc->count > 0)
    {
        opt_jiffydos_allow = 0;
        opt_jiffydos = 0;
        runstate = RUNSTATE_LOADED_CONTENT;
    }
#endif

    // Logging
    if (dc->count > 0)
    {
        if (dc->unit == 1)
            log_cb(RETRO_LOG_INFO, "Image list is active for tape\n");
        else if (dc->unit >= 8 && dc->unit <= 11)
            log_cb(RETRO_LOG_INFO, "Image list is active for drive #%d\n", dc->unit);
        else if (dc->unit == 0)
            log_cb(RETRO_LOG_INFO, "Image list is active for cart\n");

        log_cb(RETRO_LOG_INFO, "Image list has %d file(s)\n", dc->count);

        for(unsigned i = 0; i < dc->count; i++)
            log_cb(RETRO_LOG_INFO, "File %d: %s\n", i+1, dc->files[i]);
    }

    // If flip list is not empty, but there is no image attached to drive, attach the first one from list.
    // This can only happen if flip list was loaded via cmd file or from m3u with #COMMAND
    if (dc->count != 0)
    {
        if (dc->unit == 1)
        {
            if ((attachedImage = tape_get_file_name()) == NULL)
            {
                attachedImage = dc->files[0];
                // Don't attach if we will autostart from it just in a moment
                if (autostartString != NULL || noautostart)
                {
                    log_cb(RETRO_LOG_INFO, "Attaching first tape %s\n", attachedImage);
                    tape_image_attach(dc->unit, attachedImage);
                }
            }
        }
        else if (dc->unit == 8)
        {
            if ((attachedImage = file_system_get_disk_name(dc->unit)) == NULL)
            {
                attachedImage = dc->files[0];
                // Don't attach if we will autostart from it just in a moment
                if (autostartString != NULL || noautostart)
                {
                    log_cb(RETRO_LOG_INFO, "Attaching first disk %s to drive #%d\n", attachedImage, dc->unit);
                    file_system_attach_disk(dc->unit, attachedImage);
                }
            }
        }
        if (dc->unit == 0)
        {
            if (attachedImage == NULL)
            {
                attachedImage = dc->files[0];
                // Don't attach if we will autostart from it just in a moment
                if (autostartString != NULL || noautostart)
                {
                    log_cb(RETRO_LOG_INFO, "Attaching first cart %s\n", attachedImage);
#if defined(__XVIC__)
                    cartridge_attach_image(CARTRIDGE_VIC20_DETECT, attachedImage);
#elif defined(__XPLUS4__)
                    cartridge_attach_image(CARTRIDGE_PLUS4_DETECT, attachedImage);
                    // No autostarting carts, otherwise gfx gets corrupted (?!)
                    noautostart = true;
#else
                    cartridge_attach_image(dc->unit, attachedImage);
#endif
                }
            }
        }
    }

    // Disable autostart only with disks or tapes
    if (noautostart && attachedImage != NULL)
       autostart_disable();

    // If there an image attached, but autostart is empty, autostart from the image
    if (autostartString == NULL && attachedImage != NULL && !noautostart)
    {
        log_cb(RETRO_LOG_INFO, "Autostarting from attached or first image %s\n", attachedImage);
        autostartString = x_strdup(attachedImage);
        autostart_autodetect(autostartString, NULL, 0, AUTOSTART_MODE_RUN);
    }

    // If vice has image attached to drive, tell libretro that the 'tray' is closed
    if (attachedImage != NULL)
    {
        dc->eject_state = false;
        display_current_image(attachedImage, true);
    }
    else
    {
        dc->eject_state = true;
        display_current_image("", false);
    }
}

void build_params()
{
    int i;

    if (PARAMCOUNT == 0)
    {
        // No game loaded - set command line to 'x64'
        Add_Option(CORE_NAME);
    }

    for (i = 0; i < PARAMCOUNT; i++)
    {
        xargv_cmd[i] = (char*)(XARGV[i]);
        log_cb(RETRO_LOG_INFO, "Arg%d: %s\n",i,XARGV[i]);
    }

    xargv_cmd[PARAMCOUNT] = NULL;
}

extern char archdep_startup_error[];

static void archdep_startup_error_log_lines()
{
    /* Message may contain several lines, log them separately for better readbility. */
    for (char *p=archdep_startup_error, *p_end;strlen(p);p=p_end)
    {
        if (!(p_end=strchr(p,'\n')))
            p_end=p+strlen(p);
        else
            *(p_end++)=0;
        log_cb(RETRO_LOG_WARN, "VICE: %s\n", p);
    }
}

int pre_main()
{
    int argc = PARAMCOUNT;

    /* start core with full params */
    build_params();

    *archdep_startup_error=0;
    if (skel_main(argc, (char**)xargv_cmd) < 0)
    {
        log_cb(RETRO_LOG_WARN, "Core startup failed with error:\n");
        archdep_startup_error_log_lines();
        log_cb(RETRO_LOG_INFO, "Core startup retry without parameters.\n");

        /* Show only first line in message to indicate something went wrong. */
        struct retro_message rmsg;
        rmsg.msg = archdep_startup_error;
        rmsg.frames = 500;
        environ_cb(RETRO_ENVIRONMENT_SET_MESSAGE, &rmsg);

        /* start core with empty params */
        xargv_cmd[0] = CORE_NAME;
        xargv_cmd[1] = NULL;
        argc = 1;

        *archdep_startup_error=0;
        if (skel_main(argc, (char**)xargv_cmd) < 0)
        {
            log_cb(RETRO_LOG_ERROR, "Core startup without parameters failed with error:\n");
            archdep_startup_error_log_lines();
            environ_cb(RETRO_ENVIRONMENT_SHUTDOWN, NULL);
        }
    }

    return 0;
}

static void update_variables(void);

extern int ui_init_finalize(void);

void reload_restart()
{
    /* Clear request */
    request_reload_restart = 0;

    /* Stop datasette */
    datasette_control(DATASETTE_CONTROL_STOP);

    /* Cleanup after previous content and reset resources */
    initcmdline_cleanup();

    /* Update resources from environment just like on fresh start of core */
    sound_volume_counter = 3;
    retro_ui_finalized = 0;
    update_variables();
    /* Some resources are not set until we call this */
    ui_init_finalize();

    /* And process command line */
    build_params();
    if (initcmdline_restart(PARAMCOUNT, (char**)xargv_cmd) < 0)
    {
        log_cb(RETRO_LOG_ERROR, "Restart failed\n");
        /* Nevermind, the core is already running */
    }

    /* Now read disk image and autostart file (may be the same or not) from vice */
    update_from_vice();
}

long GetTicks(void) {
   return retro_now;
}

#include "libretro-keyboard.i"
int keyId(const char *val)
{
   int i=0;
   while (keyDesc[i]!=NULL)
   {
      if (!strcmp(keyDesc[i],val))
         return keyVal[i];
      i++;
   }
   return 0;
}

void retro_set_environment(retro_environment_t cb)
{
   static const struct retro_controller_description p1_controllers[] = {
      { "Joystick", RETRO_DEVICE_VICE_JOYSTICK },
      { "Keyboard", RETRO_DEVICE_VICE_KEYBOARD },
      { "None", RETRO_DEVICE_NONE },
   };
   static const struct retro_controller_description p2_controllers[] = {
      { "Joystick", RETRO_DEVICE_VICE_JOYSTICK },
      { "Keyboard", RETRO_DEVICE_VICE_KEYBOARD },
      { "None", RETRO_DEVICE_NONE },
   };
   static const struct retro_controller_description p3_controllers[] = {
      { "Joystick", RETRO_DEVICE_VICE_JOYSTICK },
      { "Keyboard", RETRO_DEVICE_VICE_KEYBOARD },
      { "None", RETRO_DEVICE_NONE },
   };
   static const struct retro_controller_description p4_controllers[] = {
      { "Joystick", RETRO_DEVICE_VICE_JOYSTICK },
      { "Keyboard", RETRO_DEVICE_VICE_KEYBOARD },
      { "None", RETRO_DEVICE_NONE },
   };
   static const struct retro_controller_description p5_controllers[] = {
      { "Joystick", RETRO_DEVICE_VICE_JOYSTICK },
      { "Keyboard", RETRO_DEVICE_VICE_KEYBOARD },
      { "None", RETRO_DEVICE_NONE },
   };

   static const struct retro_controller_info ports[] = {
      { p1_controllers, 3 }, // port 1
      { p2_controllers, 3 }, // port 2
      { p3_controllers, 3 }, // port 3
      { p4_controllers, 3 }, // port 4
      { p5_controllers, 3 }, // port 5
      { NULL, 0 }
   };

   static struct retro_core_option_definition core_options[] =
   {
#if defined(__XVIC__)
      {
         "vice_vic20_model",
         "Model",
         "",
         {
            { "VIC20 PAL", "VIC-20 PAL" },
            { "VIC20 NTSC", "VIC-20 NTSC" },
            { "VIC21", "Super VIC (+16K) NTSC" },
            { NULL, NULL },
         },
         "VIC20 PAL"
      },
      {
         "vice_vic20_memory_expansions",
         "Memory Expansions",
         "Expansion change will reset the system.",
         {
            { "none", "disabled" },
            { "3kB", "3KB" },
            { "8kB", "8KB" },
            { "16kB", "16KB" },
            { "24kB", "24KB" },
            { "all", "All" },
            { NULL, NULL },
         },
         "none"
      },
#elif defined(__XPLUS4__)
      {
         "vice_plus4_model",
         "Model",
         "",
         {
            { "C16 PAL", "C16 PAL" },
            { "C16 NTSC", "C16 NTSC" },
            { "PLUS4 PAL", "Plus/4 PAL" },
            { "PLUS4 NTSC", "Plus/4 NTSC" },
            { "V364 NTSC", "V364 NTSC" },
            { "232 NTSC", "C232 NTSC" },
            { NULL, NULL }
         },
         "PLUS4 PAL"
      },
#elif defined(__X128__)
      {
         "vice_c128_model",
         "Model",
         "",
         {
            { "C128 PAL", "C128 PAL" },
            { "C128 NTSC", "C128 NTSC" },
            { "C128 DCR PAL", "C128DCR PAL" },
            { "C128 DCR NTSC", "C128DCR NTSC" },
            { NULL, NULL },
         },
         "C128 PAL"
      },
      {
         "vice_c128_video_output",
         "Video Output",
         "",
         {
            { "VICII", "VIC-II (40 cols)" },
            { "VDC", "VDC (80 cols)" },
            { NULL, NULL },
         },
         "VICII"
      },
      {
         "vice_c128_go64",
         "GO64",
         "Start in C64 compatibility mode.\nFull restart required.",
         {
            { "disabled", NULL },
            { "enabled", NULL },
            { NULL, NULL },
         },
         "disabled"
      },
#elif defined(__XPET__)
      {
         "vice_pet_model",
         "Model",
         "",
         {
            { "2001", "PET 2001" },
            { "3008", "PET 3008" },
            { "3016", "PET 3016" },
            { "3032", "PET 3032" },
            { "3032B", "PET 3032B" },
            { "4016", "PET 4016" },
            { "4032", "PET 4032" },
            { "4032B", "PET 4032B" },
            { "8032", "PET 8032" },
            { "8096", "PET 8096" },
            { "8296", "PET 8296" },
            { "SUPERPET", "Super PET" },
            { NULL, NULL },
         },
         "8032"
      },
#elif defined(__XCBM2__)
      {
         "vice_cbm2_model",
         "Model",
         "",
         {
            { "610 PAL", "CBM 610 PAL" },
            { "610 NTSC", "CBM 610 NTSC" },
            { "620 PAL", "CBM 620 PAL" },
            { "620 NTSC", "CBM 620 NTSC" },
            //{ "620PLUS PAL", "CBM 620+ PAL" },
            //{ "620PLUS NTSC", "CBM 620+ NTSC" },
            { "710 NTSC", "CBM 710 NTSC" },
            { "720 NTSC", "CBM 720 NTSC" },
            { "720PLUS NTSC", "CBM 720+ NTSC" },
            { NULL, NULL },
         },
         "610 PAL"
      },
#elif defined(__XCBM5x0__)
      {
         "vice_cbm5x0_model",
         "Model",
         "",
         {
            { "510 PAL", "CBM 510 PAL" },
            { "510 NTSC", "CBM 510 NTSC" },
            { NULL, NULL },
         },
         "510 PAL"
      },
#else
      {
         "vice_c64_model",
         "Model",
         "'Automatic' switches region per filename and directory tags.",
         {
            { "C64 PAL auto", "C64 PAL Automatic" },
            { "C64 NTSC auto", "C64 NTSC Automatic" },
            { "C64C PAL auto", "C64C PAL Automatic" },
            { "C64C NTSC auto", "C64C NTSC Automatic" },
            { "C64 PAL", "C64 PAL" },
            { "C64 NTSC", "C64 NTSC" },
            { "C64C PAL", "C64C PAL" },
            { "C64C NTSC", "C64C NTSC" },
            { "C64SX PAL", "SX-64 PAL" },
            { "C64SX NTSC", "SX-64 NTSC" },
            { "PET64 PAL", "Educator 64 (PET 64) PAL" },
            { "PET64 NTSC", "Educator 64 (PET 64) NTSC" },
            { "C64 GS PAL", "C64 Games System PAL" },
            { "C64 JAP NTSC", "C64 Japanese NTSC" },
            //{ "C64 OLD PAL", NULL },
            //{ "C64 PAL N", NULL },
            //{ "C64 OLD NTSC", NULL },
            { NULL, NULL },
         },
         "C64 PAL auto"
      },
#endif
#if defined(__X64__) || defined(__X64SC__) || defined(__X128__) || defined(__XSCPU64__)
      {
         "vice_jiffydos",
         "Use JiffyDOS",
#if defined(__X64__) || defined(__X64SC__) || defined(__XSCPU64__)
         "For D64, D71 & D81 disk images only!\nROMs required in 'system/vice':\n- 'JiffyDOS_C64.bin'\n- 'JiffyDOS_1541-II.bin'\n- 'JiffyDOS_1571_repl310654.bin'\n- 'JiffyDOS_1581.bin'",
#elif defined(__X128__)
         "For D64, D71 & D81 disk images only!\nROMs required in 'system/vice':\n- 'JiffyDOS_C128.bin'\n- 'JiffyDOS_C64.bin' (GO64)\n- 'JiffyDOS_1541-II.bin'\n- 'JiffyDOS_1571_repl310654.bin'\n- 'JiffyDOS_1581.bin'",
#endif
         {
            { "disabled", NULL },
            { "enabled", NULL },
            { NULL, NULL },
         },
         "disabled"
      },
#endif
      {
         "vice_read_vicerc",
         "Read 'vicerc'",
         "Process 'system/vice/vicerc'. The config file can be used to set other options, such as cartridges.",
         {
            { "disabled", NULL },
            { "enabled", NULL },
            { NULL, NULL },
         },
         "enabled"
      },
      {
         "vice_reset",
         "Reset Type",
         "'Autostart' does hard reset and reruns content. 'Soft' keeps some code in memory, 'Hard' erases all memory. 'Freeze' is for cartridges.",
         {
            { "Autostart", NULL },
            { "Soft", NULL },
            { "Hard", NULL },
            { "Freeze", NULL },
            { NULL, NULL },
         },
         "Autostart"
      },
      {
         "vice_autostart",
         "Autostart",
         "'ON' always runs content, 'OFF' runs only PRG/CRT, 'Warp' turns warp mode on during autostart loading.",
         {
            { "disabled", NULL },
            { "enabled", NULL },
            { "warp", "Warp" },
            { NULL, NULL },
         },
         "enabled"
      },
      {
         "vice_autoloadwarp",
         "Automatic Load Warp",
         "Toggles warp mode always during disk and tape loading. Drive Sound Emulation will be muted.",
         {
            { "disabled", NULL },
            { "enabled", NULL },
            { NULL, NULL },
         },
         "disabled"
      },
      {
         "vice_drive_true_emulation",
         "True Drive Emulation",
         "Loads much slower, but some games need it.",
         {
            { "disabled", NULL },
            { "enabled", NULL },
            { NULL, NULL },
         },
         "enabled"
      },
      {
         "vice_work_disk",
         "Global Work Disk",
         "Global disk in device 8 will only be inserted when the core is started without content.",
         {
            { "disabled", NULL },
            { "8_d64", "D64 - 664 blocks, 170KB - Device 8" },
            { "9_d64", "D64 - 664 blocks, 170KB - Device 9" },
            { "8_d71", "D71 - 1328 blocks, 340KB - Device 8" },
            { "9_d71", "D71 - 1328 blocks, 340KB - Device 9" },
            { "8_d81", "D81 - 3160 blocks, 800KB - Device 8" },
            { "9_d81", "D81 - 3160 blocks, 800KB - Device 9" },
            { NULL, NULL },
         },
         "disabled"
      },
      {
         "vice_video_options_display",
         "Show Video Options",
         "Core options page refresh required.",
         {
            { "disabled", NULL },
            { "enabled", NULL },
            { NULL, NULL },
         },
         "disabled"
      },
#if defined(__X64__) || defined(__X64SC__) || defined(__X128__) || defined(__XSCPU64__) || defined(__XCBM5x0__) || defined(__XVIC__) || defined(__XPLUS4__)
      {
         "vice_aspect_ratio",
         "Video > Pixel Aspect Ratio",
         "",
         {
            { "auto", "Automatic" },
            { "pal", "PAL" },
            { "ntsc", "NTSC" },
            { "raw", "1:1" },
            { NULL, NULL },
         },
         "auto"
      },
      {
         "vice_zoom_mode",
         "Video > Zoom Mode",
         "Crops the borders to fit various host screens. Requirements in RetroArch settings:\n- Aspect Ratio: Core provided,\n- Integer Scale: Off.",
         {
            { "none", "disabled" },
            { "small", "Small" },
            { "medium", "Medium" },
            { "maximum", "Maximum" },
            { "manual", "Manual" },
            { NULL, NULL },
         },
         "none"
      },
      {
         "vice_zoom_mode_crop",
         "Video > Zoom Mode Crop",
         "Use 'Horizontal + Vertical' & 'Maximum' to remove borders completely. Ignored with 'Manual' zoom.",
         {
            { "both", "Horizontal + Vertical" },
            { "horizontal", "Horizontal" },
            { "vertical", "Vertical" },
            { "16:9", "16:9" },
            { "16:10", "16:10" },
            { "4:3", "4:3" },
            { "5:4", "5:4" },
            { NULL, NULL },
         },
         "both"
      },
      {
         "vice_manual_crop_top",
         "Video > Manual Crop Top",
#if defined(__X64__) || defined(__X64SC__) || defined(__X128__) || defined(__XSCPU64__) || defined(__XCBM5x0__)
         "VIC-II top border height:\n- 35px PAL\n- 23px NTSC",
#elif defined(__XVIC__)
         "VIC top border height:\n- 48px PAL\n- 22px NTSC",
#elif defined(__XPLUS4__)
         "TED top border height:\n- 40px PAL\n- 18px NTSC",
#endif
         MANUAL_CROP_OPTIONS,
         "0",
      },
      {
         "vice_manual_crop_bottom",
         "Video > Manual Crop Bottom",
#if defined(__X64__) || defined(__X64SC__) || defined(__X128__) || defined(__XSCPU64__) || defined(__XCBM5x0__)
         "VIC-II bottom border height:\n- 37px PAL\n- 24px NTSC",
#elif defined(__XVIC__)
         "VIC bottom border height:\n- 52px PAL\n- 28px NTSC",
#elif defined(__XPLUS4__)
         "TED bottom border height:\n- 48px PAL\n- 24px NTSC",
#endif
         MANUAL_CROP_OPTIONS,
         "0",
      },
      {
         "vice_manual_crop_left",
         "Video > Manual Crop Left",
#if defined(__X64__) || defined(__X64SC__) || defined(__X128__) || defined(__XSCPU64__) || defined(__XCBM5x0__)
         "VIC-II left border width:\n- 32px",
#elif defined(__XVIC__)
         "VIC left border width:\n- 48px PAL\n- 32px NTSC",
#elif defined(__XPLUS4__)
         "TED left border width:\n- 32px",
#endif
         MANUAL_CROP_OPTIONS,
         "0",
      },
      {
         "vice_manual_crop_right",
         "Video > Manual Crop Right",
#if defined(__X64__) || defined(__X64SC__) || defined(__X128__) || defined(__XSCPU64__) || defined(__XCBM5x0__)
         "VIC-II right border width:\n- 32px",
#elif defined(__XVIC__)
         "VIC right border width:\n- 48px PAL\n- 16px NTSC",
#elif defined(__XPLUS4__)
         "TED right border width:\n- 32px",
#endif
         MANUAL_CROP_OPTIONS,
         "0",
      },
#endif
      {
         "vice_statusbar",
         "Video > Statusbar Mode",
         "- Full: Joyports + Current image + LEDs\n- Basic: Current image + LEDs\n- Minimal: Track number + FPS hidden",
         {
            { "bottom", "Bottom Full" },
            { "bottom_minimal", "Bottom Full Minimal" },
            { "bottom_basic", "Bottom Basic" },
            { "bottom_basic_minimal", "Bottom Basic Minimal" },
            { "top", "Top Full" },
            { "top_minimal", "Top Full Minimal" },
            { "top_basic", "Top Basic" },
            { "top_basic_minimal", "Top Basic Minimal" },
            { NULL, NULL },
         },
         "bottom"
      },
      {
         "vice_vkbd_theme",
         "Video > Virtual KBD Theme",
         "By default, the keyboard comes up with RetroPad Select or F11.",
         {
            { "0", "C64" },
            { "1", "C64C" },
            { "2", "Dark" },
            { "3", "Light" },
            { NULL, NULL },
         },
         "0"
      },
      {
         "vice_vkbd_alpha",
         "Video > Virtual KBD Transparency",
         "Keyboard transparency can be toggled with RetroPad A.",
         {
            { "0\%", NULL },
            { "5\%", NULL },
            { "10\%", NULL },
            { "15\%", NULL },
            { "20\%", NULL },
            { "25\%", NULL },
            { "30\%", NULL },
            { "35\%", NULL },
            { "40\%", NULL },
            { "45\%", NULL },
            { "50\%", NULL },
            { "55\%", NULL },
            { "60\%", NULL },
            { "65\%", NULL },
            { "70\%", NULL },
            { "75\%", NULL },
            { "80\%", NULL },
            { "85\%", NULL },
            { "90\%", NULL },
            { "95\%", NULL },
            { NULL, NULL },
         },
         "20\%"
      },
      {
         "vice_gfx_colors",
         "Video > Color Depth",
         "24-bit is slower and not available on all platforms. Full restart required.",
         {
            { "16bit", "Thousands (16-bit)" },
            { "24bit", "Millions (24-bit)" },
            { NULL, NULL },
         },
         "16bit"
      },
#if defined(__XVIC__)
      {
         "vice_vic20_external_palette",
         "Video > VIC Color Palette",
         "'Colodore' is recommended for the most accurate colors.",
         {
            { "default", "Internal" },
            { "colodore_vic", "Colodore" },
            { "mike-pal", "Mike (PAL)" },
            { "mike-ntsc", "Mike (NTSC)" },
            { "vice", "Vice" },
            { NULL, NULL },
         },
         "colodore_vic"
      },
#elif defined(__XPLUS4__)
      {
         "vice_plus4_external_palette",
         "Video > TED Color Palette",
         "'Colodore' is recommended for the most accurate colors.",
         {
            { "default", "Internal" },
            { "colodore_ted", "Colodore" },
            { "yape-pal", "Yape (PAL)" },
            { "yape-ntsc", "Yape (NTSC)" },
            { NULL, NULL },
         },
         "colodore_ted"
      },
#elif defined(__XPET__)
      {
         "vice_pet_external_palette",
         "Video > CRTC Color Palette",
         "",
         {
            { "default", "Internal" },
            { "amber", "Amber" },
            { "green", "Green" },
            { "white", "White" },
            { NULL, NULL },
         },
         "default"
      },
#elif defined(__XCBM2__)
      {
         "vice_cbm2_external_palette",
         "Video > CRTC Color Palette",
         "",
         {
            { "default", "Internal" },
            { "amber", "Amber" },
            { "green", "Green" },
            { "white", "White" },
            { NULL, NULL },
         },
         "default"
      },
#else
      {
         "vice_external_palette",
         "Video > VIC-II Color Palette",
         "'Colodore' is recommended for most accurate colors.",
         {
            { "default", "Internal" },
            { "colodore", "Colodore" },
            { "pepto-pal", "Pepto (PAL)" },
            //{ "pepto-palold", "Pepto (old PAL)" },
            { "pepto-ntsc", "Pepto (NTSC)" },
            { "pepto-ntsc-sony", "Pepto (NTSC, Sony)" },
            { "cjam", "ChristopherJam" },
            { "c64hq", "C64HQ" },
            { "c64s", "C64S" },
            { "ccs64", "CCS64" },
            { "community-colors", "Community Colors" },
            { "deekay", "Deekay" },
            { "frodo", "Frodo" },
            { "godot", "Godot" },
            { "pc64", "PC64" },
            { "ptoing", "Ptoing" },
            { "rgb", "RGB" },
            { "vice", "VICE" },
            { NULL, NULL },
         },
         "colodore"
      },
#endif
#if defined(__X64__) || defined(__X64SC__) || defined(__X128__) || defined(__XSCPU64__) || defined(__XCBM5x0__) || defined(__XVIC__) || defined(__XPLUS4__)
      {
#if defined(__X64__) || defined(__X64SC__) || defined(__X128__) || defined(__XSCPU64__) || defined(__XCBM5x0__)
         "vice_vicii_color_gamma",
         "Video > VIC-II Color Gamma",
#elif defined(__XVIC__)
         "vice_vic_color_gamma",
         "Video > VIC Color Gamma",
#elif defined(__XPLUS4__)
         "vice_ted_color_gamma",
         "Video > TED Color Gamma",
#endif
         "Gamma for the internal palette.",
         {
            { "1000", "1.00" },
            { "1100", "1.10" },
            { "1200", "1.20" },
            { "1300", "1.30" },
            { "1400", "1.40" },
            { "1500", "1.50" },
            { "1600", "1.60" },
            { "1700", "1.70" },
            { "1800", "1.80" },
            { "1900", "1.90" },
            { "2000", "2.00" },
            { "2100", "2.10" },
            { "2200", "2.20" },
            { "2300", "2.30" },
            { "2400", "2.40" },
            { "2500", "2.50" },
            { "2600", "2.60" },
            { "2700", "2.70" },
            { "2800", "2.80" },
            { "2900", "2.90" },
            { "3000", "3.00" },
            { "3100", "3.10" },
            { "3200", "3.20" },
            { "3300", "3.30" },
            { "3400", "3.40" },
            { "3500", "3.50" },
            { "3600", "3.60" },
            { "3700", "3.70" },
            { "3800", "3.80" },
            { "3900", "3.90" },
            { "4000", "4.00" },
            { NULL, NULL },
         },
         "2800"
      },
      {
#if defined(__X64__) || defined(__X64SC__) || defined(__X128__) || defined(__XSCPU64__) || defined(__XCBM5x0__)
         "vice_vicii_color_saturation",
         "Video > VIC-II Color Saturation",
#elif defined(__XVIC__)
         "vice_vic_color_saturation",
         "Video > VIC Color Saturation",
#elif defined(__XPLUS4__)
         "vice_ted_color_saturation",
         "Video > TED Color Saturation",
#endif
         "Saturation for the internal palette.",
         {
            { "200", "10\%" },
            { "250", "12.5\%" },
            { "300", "15\%" },
            { "350", "17.5\%" },
            { "400", "20\%" },
            { "450", "22.5\%" },
            { "500", "25\%" },
            { "550", "27.5\%" },
            { "600", "30\%" },
            { "650", "32.5\%" },
            { "700", "35\%" },
            { "750", "37.5\%" },
            { "800", "40\%" },
            { "850", "42.5\%" },
            { "900", "45\%" },
            { "950", "47.5\%" },
            { "1000", "50\%" },
            { "1050", "52.5\%" },
            { "1100", "55\%" },
            { "1150", "57.5\%" },
            { "1200", "60\%" },
            { "1250", "62.5\%" },
            { "1300", "65\%" },
            { "1350", "67.5\%" },
            { "1400", "70\%" },
            { "1450", "72.5\%" },
            { "1500", "75\%" },
            { "1550", "77.5\%" },
            { "1600", "80\%" },
            { "1650", "82.5\%" },
            { "1700", "85\%" },
            { "1750", "87.5\%" },
            { "1800", "90\%" },
            { "1850", "92.5\%" },
            { "1900", "95\%" },
            { "1950", "97.5\%" },
            { "2000", "100\%" },
            { NULL, NULL },
         },
         "1000"
      },
      {
#if defined(__X64__) || defined(__X64SC__) || defined(__X128__) || defined(__XSCPU64__) || defined(__XCBM5x0__)
         "vice_vicii_color_contrast",
         "Video > VIC-II Color Contrast",
#elif defined(__XVIC__)
         "vice_vic_color_contrast",
         "Video > VIC Color Contrast",
#elif defined(__XPLUS4__)
         "vice_ted_color_contrast",
         "Video > TED Color Contrast",
#endif
         "Contrast for the internal palette.",
         {
            { "200", "10\%" },
            { "250", "12.5\%" },
            { "300", "15\%" },
            { "350", "17.5\%" },
            { "400", "20\%" },
            { "450", "22.5\%" },
            { "500", "25\%" },
            { "550", "27.5\%" },
            { "600", "30\%" },
            { "650", "32.5\%" },
            { "700", "35\%" },
            { "750", "37.5\%" },
            { "800", "40\%" },
            { "850", "42.5\%" },
            { "900", "45\%" },
            { "950", "47.5\%" },
            { "1000", "50\%" },
            { "1050", "52.5\%" },
            { "1100", "55\%" },
            { "1150", "57.5\%" },
            { "1200", "60\%" },
            { "1250", "62.5\%" },
            { "1300", "65\%" },
            { "1350", "67.5\%" },
            { "1400", "70\%" },
            { "1450", "72.5\%" },
            { "1500", "75\%" },
            { "1550", "77.5\%" },
            { "1600", "80\%" },
            { "1650", "82.5\%" },
            { "1700", "85\%" },
            { "1750", "87.5\%" },
            { "1800", "90\%" },
            { "1850", "92.5\%" },
            { "1900", "95\%" },
            { "1950", "97.5\%" },
            { "2000", "100\%" },
            { NULL, NULL },
         },
         "1200"
      },
      {
#if defined(__X64__) || defined(__X64SC__) || defined(__X128__) || defined(__XSCPU64__) || defined(__XCBM5x0__)
         "vice_vicii_color_brightness",
         "Video > VIC-II Color Brightness",
#elif defined(__XVIC__)
         "vice_vic_color_brightness",
         "Video > VIC Color Brightness",
#elif defined(__XPLUS4__)
         "vice_ted_color_brightness",
         "Video > TED Color Brightness",
#endif
         "Brightness for the internal palette.",
         {
            { "200", "10\%" },
            { "250", "12.5\%" },
            { "300", "15\%" },
            { "350", "17.5\%" },
            { "400", "20\%" },
            { "450", "22.5\%" },
            { "500", "25\%" },
            { "550", "27.5\%" },
            { "600", "30\%" },
            { "650", "32.5\%" },
            { "700", "35\%" },
            { "750", "37.5\%" },
            { "800", "40\%" },
            { "850", "42.5\%" },
            { "900", "45\%" },
            { "950", "47.5\%" },
            { "1000", "50\%" },
            { "1050", "52.5\%" },
            { "1100", "55\%" },
            { "1150", "57.5\%" },
            { "1200", "60\%" },
            { "1250", "62.5\%" },
            { "1300", "65\%" },
            { "1350", "67.5\%" },
            { "1400", "70\%" },
            { "1450", "72.5\%" },
            { "1500", "75\%" },
            { "1550", "77.5\%" },
            { "1600", "80\%" },
            { "1650", "82.5\%" },
            { "1700", "85\%" },
            { "1750", "87.5\%" },
            { "1800", "90\%" },
            { "1850", "92.5\%" },
            { "1900", "95\%" },
            { "1950", "97.5\%" },
            { "2000", "100\%" },
            { NULL, NULL },
         },
         "900"
      },
#endif
      {
         "vice_audio_options_display",
         "Show Audio Options",
         "Core options page refresh required.",
         {
            { "disabled", NULL },
            { "enabled", NULL },
            { NULL, NULL },
         },
         "disabled"
      },
      {
         "vice_drive_sound_emulation",
         "Audio > Drive Sound Emulation",
         "Emulates the iconic floppy drive sounds.\n- True Drive Emulation & D64 or D71 disk image required.",
         {
            { "disabled", NULL },
            { "10\%", "10\% volume" },
            { "15\%", "15\% volume" },
            { "20\%", "20\% volume" },
            { "25\%", "25\% volume" },
            { "30\%", "30\% volume" },
            { "35\%", "35\% volume" },
            { "40\%", "40\% volume" },
            { "45\%", "45\% volume" },
            { "50\%", "50\% volume" },
            { "55\%", "55\% volume" },
            { "60\%", "60\% volume" },
            { "65\%", "65\% volume" },
            { "70\%", "70\% volume" },
            { "75\%", "75\% volume" },
            { "80\%", "80\% volume" },
            { "85\%", "85\% volume" },
            { "90\%", "90\% volume" },
            { "95\%", "95\% volume" },
            { "100\%", "100\% volume" },
            { NULL, NULL },
         },
         "20\%"
      },
#if defined(__X64__) || defined(__X64SC__) || defined(__X128__) || defined(__XSCPU64__) || defined(__XCBM5x0__) || defined(__XVIC__) || defined(__XPLUS4__)
      {
         "vice_audio_leak_emulation",
#if defined(__X64__) || defined(__X64SC__) || defined(__X128__) || defined(__XSCPU64__) || defined(__XCBM5x0__)
         "Audio > VIC-II Audio Leak Emulation",
#elif defined(__XVIC__)
         "Audio > VIC Audio Leak Emulation",
#elif defined(__XPLUS4__)
         "Audio > TED Audio Leak Emulation",
#endif
         "",
         {
            { "disabled", NULL },
            { "1", "100\% volume" },
            { "2", "200\% volume" },
            { "3", "300\% volume" },
            { "4", "400\% volume" },
            { "5", "500\% volume" },
            { "6", "600\% volume" },
            { "7", "700\% volume" },
            { "8", "800\% volume" },
            { "9", "900\% volume" },
            { "10", "1000\% volume" },
            { NULL, NULL },
         },
         "disabled"
      },
#endif
      {
         "vice_sound_sample_rate",
         "Audio > Output Sample Rate",
         "Slightly higher quality or higher performance.",
         {
            { "22050", NULL },
            { "44100", NULL },
            { "48000", NULL },
            { "96000", NULL },
            { NULL, NULL },
         },
         "48000"
      },
#if !defined(__XPET__) && !defined(__XPLUS4__) && !defined(__XVIC__)
      {
         "vice_sid_engine",
         "Audio > SID Engine",
         "ReSID is accurate but slower.",
         {
            { "FastSID", NULL },
            { "ReSID", NULL },
            { "ReSID-3.3", NULL },
            { "ReSID-FP", NULL },
            { NULL, NULL },
         },
         "ReSID"
      },
      {
         "vice_sid_model",
         "Audio > SID Model",
         "The original C64 uses 6581, C64C uses 8580.",
         {
            { "Default", NULL },
            { "6581", NULL },
            { "8580", NULL },
            { "8580RD", "8580 ReSID + Digi Boost" },
            { NULL, NULL },
         },
         "Default"
      },
      {
         "vice_sid_extra",
         "Audio > SID Extra",
         "Second SID base address.",
         {
            { "disabled", NULL },
            { "0xd420", "$D420" },
            { "0xd500", "$D500" },
            { "0xde00", "$DE00" },
            { "0xdf00", "$DF00" },
            { NULL, NULL },
         },
         "disabled"
      },
      {
         "vice_resid_sampling",
         "Audio > ReSID Sampling",
         "'Resampling' provides best quality. 'Fast' improves performance dramatically on PS Vita.",
         {
            { "Fast", NULL },
            { "Interpolation", NULL },
            { "Resampling", NULL },
            { "Fast resampling", NULL },
            { NULL, NULL },
         },
#if defined(PSP) || defined(VITA) || defined(__SWITCH__)
         "Fast"
#else
         "Resampling"
#endif
      },
      {
         "vice_resid_passband",
         "Audio > ReSID Filter Passband",
         "Parameters for SID Filter.",
         {
            { "0", NULL },
            { "10", NULL },
            { "20", NULL },
            { "30", NULL },
            { "40", NULL },
            { "50", NULL },
            { "60", NULL },
            { "70", NULL },
            { "80", NULL },
            { "90", NULL },
            { NULL, NULL },
         },
         "90"
      },
      {
         "vice_resid_gain",
         "Audio > ReSID Filter Gain",
         "Parameters for SID Filter.",
         {
            { "90", NULL },
            { "91", NULL },
            { "92", NULL },
            { "93", NULL },
            { "94", NULL },
            { "95", NULL },
            { "96", NULL },
            { "97", NULL },
            { "98", NULL },
            { "99", NULL },
            { "100", NULL },
            { NULL, NULL },
         },
         "97"
      },
      {
         "vice_resid_filterbias",
         "Audio > ReSID Filter Bias",
         "Parameters for SID Filter.",
         {
            { "-5000", NULL },
            { "-4500", NULL },
            { "-4000", NULL },
            { "-3500", NULL },
            { "-3000", NULL },
            { "-2500", NULL },
            { "-2000", NULL },
            { "-1500", NULL },
            { "-1000", NULL },
            { "-500", NULL },
            { "0", NULL },
            { "500", NULL },
            { "1000", NULL },
            { "1500", NULL },
            { "2000", NULL },
            { "2500", NULL },
            { "3000", NULL },
            { "3500", NULL },
            { "4000", NULL },
            { "4500", NULL },
            { "5000", NULL },
            { NULL, NULL },
         },
         "500"
      },
      {
         "vice_resid_8580filterbias",
         "Audio > ReSID Filter 8580 Bias",
         "Parameters for SID Filter.",
         {
            { "-5000", NULL },
            { "-4500", NULL },
            { "-4000", NULL },
            { "-3500", NULL },
            { "-3000", NULL },
            { "-2500", NULL },
            { "-2000", NULL },
            { "-1500", NULL },
            { "-1000", NULL },
            { "-500", NULL },
            { "0", NULL },
            { "500", NULL },
            { "1000", NULL },
            { "1500", NULL },
            { "2000", NULL },
            { "2500", NULL },
            { "3000", NULL },
            { "3500", NULL },
            { "4000", NULL },
            { "4500", NULL },
            { "5000", NULL },
            { NULL, NULL },
         },
         "1500"
      },
#endif
#if !defined(__XPET__) && !defined(__XCBM2__) && !defined(__XCBM5x0__) && !defined(__XVIC__)
      {
         "vice_analogmouse_deadzone",
         "Analog Stick Mouse Deadzone",
         "",
         {
            { "0", "0\%" },
            { "5", "5\%" },
            { "10", "10\%" },
            { "15", "15\%" },
            { "20", "20\%" },
            { "25", "25\%" },
            { "30", "30\%" },
            { "35", "35\%" },
            { "40", "40\%" },
            { "45", "45\%" },
            { "50", "50\%" },
            { NULL, NULL },
         },
         "15"
      },
      {
         "vice_analogmouse_speed",
         "Analog Stick Mouse Speed",
         "",
         {
            { "0.5", "50\%" },
            { "0.6", "60\%" },
            { "0.7", "70\%" },
            { "0.8", "80\%" },
            { "0.9", "90\%" },
            { "1.0", "100\%" },
            { "1.1", "110\%" },
            { "1.2", "120\%" },
            { "1.3", "130\%" },
            { "1.4", "140\%" },
            { "1.5", "150\%" },
            { NULL, NULL },
         },
         "1.0"
      },
      {
         "vice_dpadmouse_speed",
         "D-Pad Mouse Speed",
         "",
         {
            { "3", "50\%" },
            { "4", "66\%" },
            { "5", "83\%" },
            { "6", "100\%" },
            { "7", "116\%" },
            { "8", "133\%" },
            { "9", "150\%" },
            { "10", "166\%" },
            { "11", "183\%" },
            { "12", "200\%" },
            { NULL, NULL },
         },
         "6"
      },
      {
         "vice_mouse_speed",
         "Mouse Speed",
         "Affects mouse speed globally.",
         {
            { "10", "10\%" },
            { "20", "20\%" },
            { "30", "30\%" },
            { "40", "40\%" },
            { "50", "50\%" },
            { "60", "60\%" },
            { "70", "70\%" },
            { "80", "80\%" },
            { "90", "90\%" },
            { "100", "100\%" },
            { "110", "110\%" },
            { "120", "120\%" },
            { "130", "130\%" },
            { "140", "140\%" },
            { "150", "150\%" },
            { "160", "160\%" },
            { "170", "170\%" },
            { "180", "180\%" },
            { "190", "190\%" },
            { "200", "200\%" },
            { NULL, NULL },
         },
         "100"
      },
#endif
#if !defined(__XCBM5x0__)
      {
         "vice_userport_joytype",
         "Userport Joystick Adapter",
         "Essential when 2 joysticks are not enough, for example IK+ Gold with 3 players.",
         {
            { "None", "disabled" },
            { "Protovision CGA", NULL },
            { "PET", NULL },
            { "Hummer", NULL },
            { "OEM", NULL },
            { "Hit", NULL }, 
            { "Kingsoft", NULL },
            { "Starbyte", NULL },
            { NULL, NULL },
         },
         "None"
      },
#endif
      {
         "vice_keyrah_keypad_mappings",
         "Keyrah Keypad Mappings",
         "Hardcoded keypad to joyport mappings for Keyrah hardware.",
         {
            { "disabled", NULL },
            { "enabled", NULL },
            { NULL, NULL },
         },
         "disabled"
      },
#if !defined(__XCBM2__) && !defined(__XCBM5x0__) && !defined(__XPET__)
      {
         "vice_keyboard_keymap",
         "Keyboard Keymap",
#if defined(__XPLUS4__)
         "User-defined keymap location is 'system/vice/PLUS4'.\n- Positional: 'sdl_pos.vkm'\n- Symbolic: 'sdl_sym.vkm'",
#elif defined(__XVIC__)
         "User-defined keymap location is 'system/vice/VIC20'.\n- Positional: 'sdl_pos.vkm'\n- Symbolic: 'sdl_sym.vkm'",
#elif defined(__X128__)
         "User-defined keymap location is 'system/vice/C128'.\n- Positional: 'sdl_pos.vkm'\n- Symbolic: 'sdl_sym.vkm'",
#elif defined(__XSCPU64__)
         "User-defined keymap location is 'system/vice/SCPU64'.\n- Positional: 'sdl_pos.vkm'\n- Symbolic: 'sdl_sym.vkm'",
#else
         "User-defined keymap location is 'system/vice/C64'.\n- Positional: 'sdl_pos.vkm'\n- Symbolic: 'sdl_sym.vkm'",
#endif
         {
            { "positional", "Positional" },
            { "symbolic", "Symbolic" },
            { "positional-user", "Positional (User-defined)" },
            { "symbolic-user", "Symbolic (User-defined)" },
            { NULL, NULL },
         },
         "positional"
      },
#endif
      {
         "vice_physical_keyboard_pass_through",
         "Physical Keyboard Pass-through",
         "Pass all physical keyboard events to the core. Disable this to prevent cursor keys and fire key from generating key events.",
         {
            { "disabled", NULL },
            { "enabled", NULL },
            { NULL, NULL },
         },
         "disabled"
      },
      {
         "vice_datasette_hotkeys",
         "Datasette Hotkeys",
         "Enable/disable all Datasette hotkeys.",
         {
            { "disabled", NULL },
            { "enabled", NULL },
            { NULL, NULL },
         },
         "disabled"
      },
      {
         "vice_mapping_options_display",
         "Show Mapping Options",
         "Show options for hotkeys & RetroPad mappings.\nCore options page refresh required.",
         {
            { "disabled", NULL },
            { "enabled", NULL },
            { NULL, NULL },
         },
         "disabled"
      },
      /* Hotkeys */
      {
         "vice_mapper_vkbd",
         "Hotkey > Toggle Virtual Keyboard",
         "Press the mapped key to toggle the virtual keyboard.",
         {{ NULL, NULL }},
         "RETROK_F11"
      },
      {
         "vice_mapper_statusbar",
         "Hotkey > Toggle Statusbar",
         "Press the mapped key to toggle the statusbar.",
         {{ NULL, NULL }},
         "RETROK_F12"
      },
#if !defined(__XPET__) && !defined(__XCBM2__) && !defined(__XCBM5x0__) && !defined(__XVIC__)
      {
         "vice_mapper_joyport_switch",
         "Hotkey > Switch Joyports",
         "Press the mapped key to switch joyports 1 & 2.\nSwitching will disable 'RetroPad Port' option until core restart.",
         {{ NULL, NULL }},
         "RETROK_RCTRL"
      },
#endif
      {
         "vice_mapper_reset",
         "Hotkey > Reset",
         "Press the mapped key to trigger reset.",
         {{ NULL, NULL }},
         "RETROK_END"
      },
#if defined(__X64__) || defined(__X64SC__) || defined(__X128__) || defined(__XSCPU64__) || defined(__XCBM5x0__) || defined(__XVIC__) || defined(__XPLUS4__)
      {
         "vice_mapper_zoom_mode_toggle",
         "Hotkey > Toggle Zoom Mode",
         "Press the mapped key to toggle zoom mode.",
         {{ NULL, NULL }},
         "---"
      },
#endif
      {
         "vice_mapper_warp_mode",
         "Hotkey > Hold Warp Mode",
         "Hold the mapped key for warp mode.",
         {{ NULL, NULL }},
         ""
      },
      /* Datasette controls */
      {
         "vice_mapper_datasette_toggle_hotkeys",
         "Hotkey > Toggle Datasette Hotkeys",
         "Press the mapped key to toggle the Datasette hotkeys.",
         {{ NULL, NULL }},
         "---"
      },
      {
         "vice_mapper_datasette_start",
         "Hotkey > Datasette Start",
         "Press start on tape.",
         {{ NULL, NULL }},
         "RETROK_UP"
      },
      {
         "vice_mapper_datasette_stop",
         "Hotkey > Datasette Stop",
         "Press stop on tape.",
         {{ NULL, NULL }},
         "RETROK_DOWN"
      },
      {
         "vice_mapper_datasette_rewind",
         "Hotkey > Datasette Rewind",
         "Press rewind on tape.",
         {{ NULL, NULL }},
         "RETROK_LEFT"
      },
      {
         "vice_mapper_datasette_forward",
         "Hotkey > Datasette Fast Forward",
         "Press fast forward on tape.",
         {{ NULL, NULL }},
         "RETROK_RIGHT"
      },
      {
         "vice_mapper_datasette_reset",
         "Hotkey > Datasette Reset",
         "Press reset on tape.",
         {{ NULL, NULL }},
         "---"
      },
      /* Button mappings */
      {
         "vice_mapper_select",
         "RetroPad Select",
         "",
         {{ NULL, NULL }},
         "TOGGLE_VKBD"
      },
      {
         "vice_mapper_start",
         "RetroPad Start",
         "VKBD: Press 'Return'. Remapping to non-keyboard keys overrides VKBD function!",
         {{ NULL, NULL }},
         "---"
      },
      {
         "vice_mapper_b",
         "RetroPad B",
         "Unmapped defaults to fire button.\nVKBD: Press key.",
         {{ NULL, NULL }},
         "---"
      },
      {
         "vice_mapper_a",
         "RetroPad A",
         "VKBD: Toggle transparency. Remapping to non-keyboard keys overrides VKBD function!",
         {{ NULL, NULL }},
         "---"
      },
      {
         "vice_mapper_y",
         "RetroPad Y",
         "VKBD: Toggle 'CapsLock'. Remapping to non-keyboard keys overrides VKBD function!",
         {{ NULL, NULL }},
         "---"
      },
      {
         "vice_mapper_x",
         "RetroPad X",
         "",
         {{ NULL, NULL }},
         "---"
      },
      {
         "vice_mapper_l",
         "RetroPad L",
         "",
         {{ NULL, NULL }},
         "---"
      },
      {
         "vice_mapper_r",
         "RetroPad R",
         "",
         {{ NULL, NULL }},
         "---"
      },
      {
         "vice_mapper_l2",
         "RetroPad L2",
         "",
         {{ NULL, NULL }},
         "RETROK_ESCAPE"
      },
      {
         "vice_mapper_r2",
         "RetroPad R2",
         "",
         {{ NULL, NULL }},
         "RETROK_RETURN"
      },
      {
         "vice_mapper_l3",
         "RetroPad L3",
         "",
         {{ NULL, NULL }},
         "---"
      },
      {
         "vice_mapper_r3",
         "RetroPad R3",
         "",
         {{ NULL, NULL }},
         "---"
      },
      /* Left Stick */
      {
         "vice_mapper_lu",
         "RetroPad L-Up",
         "Mapping for left analog stick up.",
         {{ NULL, NULL }},
         "---"
      },
      {
         "vice_mapper_ld",
         "RetroPad L-Down",
         "Mapping for left analog stick down.",
         {{ NULL, NULL }},
         "---"
      },
      {
         "vice_mapper_ll",
         "RetroPad L-Left",
         "Mapping for left analog stick left.",
         {{ NULL, NULL }},
         "---"
      },
      {
         "vice_mapper_lr",
         "RetroPad L-Right",
         "Mapping for left analog stick right.",
         {{ NULL, NULL }},
         "---"
      },
      /* Right Stick */
      {
         "vice_mapper_ru",
         "RetroPad R-Up",
         "Mapping for right analog stick up.",
         {{ NULL, NULL }},
         "---"
      },
      {
         "vice_mapper_rd",
         "RetroPad R-Down",
         "Mapping for right analog stick down.",
         {{ NULL, NULL }},
         "---"
      },
      {
         "vice_mapper_rl",
         "RetroPad R-Left",
         "Mapping for right analog stick left.",
         {{ NULL, NULL }},
         "---"
      },
      {
         "vice_mapper_rr",
         "RetroPad R-Right",
         "Mapping for right analog stick right.",
         {{ NULL, NULL }},
         "---"
      },
      /* Turbo Fire */
      {
         "vice_turbo_fire_button",
         "RetroPad Turbo Fire",
         "Replaces the mapped button with a turbo fire button.",
         {
            { "disabled", NULL },
            { "A", "RetroPad A" },
            { "Y", "RetroPad Y" },
            { "X", "RetroPad X" },
            { "L", "RetroPad L" },
            { "R", "RetroPad R" },
            { "L2", "RetroPad L2" },
            { "R2", "RetroPad R2" },
            { NULL, NULL },
         },
         "disabled"
      },
      {
         "vice_turbo_pulse",
         "RetroPad Turbo Pulse",
         "Frames in a button cycle. 2 equals button press on a frame and release on the next frame.",
         {
            { "2", NULL },
            { "4", NULL },
            { "6", NULL },
            { "8", NULL },
            { "10", NULL },
            { "12", NULL },
            { NULL, NULL },
         },
         "4"
      },
#if !defined(__XPET__) && !defined(__XCBM2__) && !defined(__XCBM5x0__) && !defined(__XVIC__)
      {
         "vice_joyport",
         "RetroPad Port",
         "Most games use port 2, some use port 1.\nFilename forcing or hotkey toggling will disable this option until core restart.",
         {
            { "Port 2", NULL },
            { "Port 1", NULL },
            { NULL, NULL },
         },
         "Port 2"
      },
      {
         "vice_joyport_type",
         "RetroPad Port Type",
         "Non-joysticks will be plugged into current port only and are controlled with the left analog stick or a real mouse. Paddles will be split to 1st and 2nd RetroPort.",
         {
            { "1", "Joystick" },
            { "2", "Paddles" },
            { "3", "Mouse (1351)" },
            { "4", "Mouse (NEOS)" },
            { "5", "Mouse (Amiga)" },
            { "6", "Trackball (Atari CX-22)" },
            { "7", "Mouse (Atari ST)" },
            { "8", "Mouse (SmartMouse)" },
            { "9", "Mouse (Micromys)" },
            { "10", "Koalapad" },
            { NULL, NULL },
         },
         "1"
      },
#endif
      {
         "vice_retropad_options",
         "RetroPad Face Button Options",
         "Rotate face buttons clockwise and/or make 2nd fire press up.",
         {
            { "disabled", "B = Fire" },
            { "jump", "B = Fire, A = Up" },
            { "rotate", "Y = Fire" },
            { "rotate_jump", "Y = Fire, B = Up" },
            { NULL, NULL },
         },
         "disabled"
      },

      { NULL, NULL, NULL, {{0}}, NULL },
   };

   /* fill in the values for all the mappers */
   int i = 0;
   int j = 0;
   int hotkey = 0;
   while (core_options[i].key) 
   {
      if (strstr(core_options[i].key, "vice_mapper_"))
      {
         /* Show different key list for hotkeys (special negatives removed) */
         if (  strstr(core_options[i].key, "vice_mapper_vkbd")
            || strstr(core_options[i].key, "vice_mapper_statusbar")
            || strstr(core_options[i].key, "vice_mapper_joyport_switch")
            || strstr(core_options[i].key, "vice_mapper_reset")
            || strstr(core_options[i].key, "vice_mapper_zoom_mode_toggle")
            || strstr(core_options[i].key, "vice_mapper_warp_mode")
            || strstr(core_options[i].key, "vice_mapper_datasette_toggle_hotkeys")
            || strstr(core_options[i].key, "vice_mapper_datasette_start")
            || strstr(core_options[i].key, "vice_mapper_datasette_stop")
            || strstr(core_options[i].key, "vice_mapper_datasette_rewind")
            || strstr(core_options[i].key, "vice_mapper_datasette_forward")
            || strstr(core_options[i].key, "vice_mapper_datasette_reset")
            )
            hotkey = 1;
         else
            hotkey = 0;

         j = 0;
         if (hotkey)
         {
             while (keyDescHotkeys[j] && j < RETRO_NUM_CORE_OPTION_VALUES_MAX - 1)
             {
                core_options[i].values[j].value = keyDescHotkeys[j];
                core_options[i].values[j].label = NULL;
                ++j;
             };
         }
         else
         {
             while (keyDesc[j] && j < RETRO_NUM_CORE_OPTION_VALUES_MAX - 1)
             {
                core_options[i].values[j].value = keyDesc[j];
                core_options[i].values[j].label = NULL;
                ++j;
             };
         }
         core_options[i].values[j].value = NULL;
         core_options[i].values[j].label = NULL;
      };
      ++i;
   }

   environ_cb = cb;
   cb(RETRO_ENVIRONMENT_SET_CONTROLLER_INFO, (void*)ports);

   unsigned version = 0;
   if (!cb(RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION, &version))
   {
      /* Only log the error if we were not called after retro_deinit */
      if (log_cb)
         log_cb(RETRO_LOG_WARN,"retro_set_environment: GET_CORE_OPTIONS_VERSION failed, not setting core-options now.\n");
   }
   else if (version == 1)
      cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS, core_options);
   else
   {
      /* Fallback for older API */

      /* Use define because C doesn't care about const. */
#define NUM_CORE_OPTIONS ( sizeof(core_options)/sizeof(core_options[0])-1 )
      static struct retro_variable variables[NUM_CORE_OPTIONS+1];

      /* Only generate variables once, it's as static as core_options */
      if (!core_options_legacy_strings)
      {
         /* First pass: Calculate size of string-buffer required */
         unsigned buf_len;
         char *buf;
         {
            unsigned alloc_len=0;
            struct retro_core_option_definition *o=core_options+NUM_CORE_OPTIONS-1;
            struct retro_variable *rv=variables+NUM_CORE_OPTIONS-1;
            for (; o>=core_options; --o, --rv)
            {
               int l=snprintf(0,0,"%s; %s",o->desc,o->default_value);
               for (struct retro_core_option_value *v=o->values;v->value;++v)
                  l+=snprintf(0,0,"|%s",v->value);
               alloc_len+=l+1;
            }
            buf=core_options_legacy_strings=(char *)malloc(alloc_len);
            buf_len=alloc_len;
         }
         /* Second pass: Fill string-buffer */
         struct retro_core_option_definition *o=core_options+NUM_CORE_OPTIONS-1;
         struct retro_variable *rv=variables+NUM_CORE_OPTIONS;
         rv->key = rv->value = 0;
         --rv;
         for (; o>=core_options; --o, --rv)
         {
            int l=snprintf(buf,buf_len,"%s; %s",o->desc,o->default_value);
            for (struct retro_core_option_value *v=o->values;v->value;++v)
               l+=snprintf(buf+l,buf_len,"|%s",v->value);
            rv->key = o->key;
            rv->value = buf;
            ++l;
            buf+=l, buf_len-=l;
         }
      }
      cb( RETRO_ENVIRONMENT_SET_VARIABLES, variables);
#undef NUM_CORE_OPTIONS
   }

   static bool allowNoGameMode;
   allowNoGameMode = true;
   environ_cb(RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME, &allowNoGameMode);
}

int log_resources_set_int(const char *name, int value)
{
    log_cb(RETRO_LOG_INFO, "Resource %s = %d\n", name, value);
    return resources_set_int(name, value);
}

int log_resources_set_string(const char *name, const char* value)
{
    log_cb(RETRO_LOG_INFO, "Resource %s = \"%s\"\n", name, value);
    return resources_set_string(name, value);
}

static void update_variables(void)
{
   struct retro_variable var;
   struct retro_core_option_display option_display;

   log_cb(RETRO_LOG_INFO, "Updating variables, UI finalized = %d\n", retro_ui_finalized);

   var.key = "vice_autostart";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (retro_ui_finalized)
      {
         if (!strcmp(var.value, "warp") && !core_opt.AutostartWarp)
            log_resources_set_int("AutostartWarp", 1);
         else if (core_opt.AutostartWarp)
            log_resources_set_int("AutostartWarp", 0);
      }

      if (!strcmp(var.value, "warp")) core_opt.AutostartWarp = 1;
      else core_opt.AutostartWarp = 0;

      if (!strcmp(var.value, "disabled")) noautostart = true;
      else noautostart = false;
   }

   var.key = "vice_autoloadwarp";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (!strcmp(var.value, "disabled")) opt_autoloadwarp = 0;
      else opt_autoloadwarp = 1;

      // Silently restore sounds when autoloadwarp is disabled and DSE is enabled
      if (retro_ui_finalized && core_opt.DriveSoundEmulation && core_opt.DriveTrueEmulation && !opt_autoloadwarp)
         resources_set_int("DriveSoundEmulationVolume", core_opt.DriveSoundEmulation);
   }

   var.key = "vice_work_disk";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      int work_disk_type = opt_work_disk_type;
      int work_disk_unit = opt_work_disk_unit;

      if (!strcmp(var.value, "disabled")) opt_work_disk_type = 0;
      else
      {
         if (strstr(var.value, "_d64")) opt_work_disk_type = DISK_IMAGE_TYPE_D64;
         else if (strstr(var.value, "_d71")) opt_work_disk_type = DISK_IMAGE_TYPE_D71;
         else if (strstr(var.value, "_d81")) opt_work_disk_type = DISK_IMAGE_TYPE_D81;

         if (strstr(var.value, "8_")) opt_work_disk_unit = 8;
         else if (strstr(var.value, "9_")) opt_work_disk_unit = 9;
      }

      if (work_disk_type != opt_work_disk_type || work_disk_unit != opt_work_disk_unit)
         request_update_work_disk = true;
   }

   var.key = "vice_drive_true_emulation";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (retro_ui_finalized)
      {
         if (!strcmp(var.value, "disabled") && core_opt.DriveTrueEmulation)
            log_resources_set_int("DriveTrueEmulation", 0);
         else if (!strcmp(var.value, "enabled") && !core_opt.DriveTrueEmulation)
            log_resources_set_int("DriveTrueEmulation", 1);
      }

      if (!strcmp(var.value, "disabled")) core_opt.DriveTrueEmulation = 0;
      else core_opt.DriveTrueEmulation = 1;

      // Silently restore sounds when TDE and DSE is enabled
      if (retro_ui_finalized && core_opt.DriveSoundEmulation && core_opt.DriveTrueEmulation)
         resources_set_int("DriveSoundEmulationVolume", core_opt.DriveSoundEmulation);
   }

   var.key = "vice_drive_sound_emulation";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      int val = atoi(var.value) * 20;

      if (retro_ui_finalized && core_opt.DriveSoundEmulation != val)
      {
         if (!strcmp(var.value, "disabled"))
         {
            log_resources_set_int("DriveSoundEmulation", 0);
            log_resources_set_int("DriveSoundEmulationVolume", 0);
         }
         else
         {
            log_resources_set_int("DriveSoundEmulation", 1);
            log_resources_set_int("DriveSoundEmulationVolume", val);
         }
      }

      core_opt.DriveSoundEmulation = val;

      // Silently mute sounds without TDE,
      // because motor sound will not stop if TDE is changed during motor spinning
      // and also with autoloadwarping, because warping is muted anyway
      if (retro_ui_finalized && core_opt.DriveSoundEmulation && (!core_opt.DriveTrueEmulation || opt_autoloadwarp))
         resources_set_int("DriveSoundEmulationVolume", 0);
   }

#if defined(__X64__) || defined(__X64SC__) || defined(__X128__) || defined(__XSCPU64__) || defined(__XCBM5x0__) || defined(__XVIC__) || defined(__XPLUS4__)
   var.key = "vice_audio_leak_emulation";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      int audioleak = 0;
      opt_audio_leak_volume=atoi(var.value);

      if (!strcmp(var.value, "disabled")) audioleak = 0;
      else audioleak = 1;

      if (retro_ui_finalized && core_opt.AudioLeak != audioleak)
#if defined(__X64__) || defined(__X64SC__) || defined(__X128__) || defined(__XSCPU64__) || defined(__XCBM5x0__)
         log_resources_set_int("VICIIAudioLeak", audioleak);
#elif defined(__XVIC__)
         log_resources_set_int("VICAudioLeak", audioleak);
#elif defined(__XPLUS4__)
         log_resources_set_int("TEDAudioLeak", audioleak);
#endif
      core_opt.AudioLeak = audioleak;
   }
#endif

   var.key = "vice_sound_sample_rate";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      core_opt.SoundSampleRate = atoi(var.value);
   }

#if defined(__XVIC__)
   var.key = "vice_vic20_model";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      int model = 0;

      if (!strcmp(var.value, "VIC20 PAL")) model = VIC20MODEL_VIC20_PAL;
      else if (!strcmp(var.value, "VIC20 NTSC")) model = VIC20MODEL_VIC20_NTSC;
      else if (!strcmp(var.value, "VIC21")) model = VIC20MODEL_VIC21;

      if (retro_ui_finalized && core_opt.Model != model)
      {
         vic20model_set(model);
         // Memory expansion needs to be reseted to get updated
         core_opt.VIC20Memory = 0xff;
      }

      core_opt.Model = model;
   }

   var.key = "vice_vic20_memory_expansions";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      int vic20mem = 0;

      if (!strcmp(var.value, "none")) vic20mem = 0;
      else if (!strcmp(var.value, "3kB")) vic20mem = 1;
      else if (!strcmp(var.value, "8kB")) vic20mem = 2;
      else if (!strcmp(var.value, "16kB")) vic20mem = 3;
      else if (!strcmp(var.value, "24kB")) vic20mem = 4;
      else if (!strcmp(var.value, "all")) vic20mem = 5;

      // Super VIC uses memory blocks 1+2 by default
      if (!vic20mem && core_opt.Model == VIC20MODEL_VIC21)
         vic20mem = 3;

      if (retro_ui_finalized && core_opt.VIC20Memory != vic20mem)
      {
         unsigned int vic_blocks = 0;
         switch (vic20mem)
         {
            case 1:
               vic_blocks |= VIC_BLK0;
               break;

            case 2:
               vic_blocks |= VIC_BLK1;
               break;

            case 3:
               vic_blocks |= VIC_BLK1;
               vic_blocks |= VIC_BLK2;
               break;

            case 4:
               vic_blocks |= VIC_BLK1;
               vic_blocks |= VIC_BLK2;
               vic_blocks |= VIC_BLK3;
               break;

            case 5:
               vic_blocks = VIC_BLK_ALL;
               break;
         }

         log_resources_set_int("RAMBlock0", (vic_blocks & VIC_BLK0) ? 1 : 0);
         log_resources_set_int("RAMBlock1", (vic_blocks & VIC_BLK1) ? 1 : 0);
         log_resources_set_int("RAMBlock2", (vic_blocks & VIC_BLK2) ? 1 : 0);
         log_resources_set_int("RAMBlock3", (vic_blocks & VIC_BLK3) ? 1 : 0);
         log_resources_set_int("RAMBlock5", (vic_blocks & VIC_BLK5) ? 1 : 0);
         machine_trigger_reset(MACHINE_RESET_MODE_HARD);
      }

      core_opt.VIC20Memory = vic20mem;
   }
#elif defined(__XPLUS4__)
   var.key = "vice_plus4_model";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      int model = 0;

      if (!strcmp(var.value, "C16 PAL")) model = PLUS4MODEL_C16_PAL;
      else if (!strcmp(var.value, "C16 NTSC")) model = PLUS4MODEL_C16_NTSC;
      else if (!strcmp(var.value, "PLUS4 PAL")) model = PLUS4MODEL_PLUS4_PAL;
      else if (!strcmp(var.value, "PLUS4 NTSC")) model = PLUS4MODEL_PLUS4_NTSC;
      else if (!strcmp(var.value, "V364 NTSC")) model = PLUS4MODEL_V364_NTSC;
      else if (!strcmp(var.value, "232 NTSC")) model = PLUS4MODEL_232_NTSC;

      if (retro_ui_finalized && core_opt.Model != model)
         plus4model_set(model);

      core_opt.Model = model;
   }
#elif defined(__X128__)
   var.key = "vice_c128_model";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      int model = 0;

      if (!strcmp(var.value, "C128 PAL")) model = C128MODEL_C128_PAL;
      else if (!strcmp(var.value, "C128 NTSC")) model = C128MODEL_C128_NTSC;
      else if (!strcmp(var.value, "C128 DCR PAL")) model = C128MODEL_C128DCR_PAL;
      else if (!strcmp(var.value, "C128 DCR NTSC")) model = C128MODEL_C128DCR_NTSC;

      if (retro_ui_finalized && core_opt.Model != model)
         c128model_set(model);

      core_opt.Model = model;
   }

   var.key = "vice_c128_video_output";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      int c128columnkey = 1;

      if (!strcmp(var.value, "VICII")) c128columnkey = 1;
      else if (!strcmp(var.value, "VDC")) c128columnkey = 0;

      if (retro_ui_finalized && core_opt.C128ColumnKey != c128columnkey)
      {
         log_resources_set_int("C128ColumnKey", c128columnkey);
         machine_trigger_reset(MACHINE_RESET_MODE_SOFT);
      }

      core_opt.C128ColumnKey = c128columnkey;
   }

   var.key = "vice_c128_go64";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      int c128go64 = 0;

      if (!strcmp(var.value, "disabled")) c128go64 = 0;
      else if (!strcmp(var.value, "enabled")) c128go64 = 1;

      // Force VIC-II with GO64
      if (c128go64)
      {
         core_opt.C128ColumnKey = 1;
         if (retro_ui_finalized)
            log_resources_set_int("C128ColumnKey", 1);
      }

      if (retro_ui_finalized && core_opt.Go64Mode != c128go64)
      {
         log_resources_set_int("Go64Mode", c128go64);
         // Skip reset for now, because going into 64 mode while running produces VDC related endless garbage, but typing GO64 works?!
         //machine_trigger_reset(MACHINE_RESET_MODE_HARD);
      }
      core_opt.Go64Mode = c128go64;
   }
#elif defined(__XPET__)
   var.key = "vice_pet_model";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      int model = 0;

      if (!strcmp(var.value, "2001")) model = PETMODEL_2001;
      else if (!strcmp(var.value, "3008")) model = PETMODEL_3008;
      else if (!strcmp(var.value, "3016")) model = PETMODEL_3016;
      else if (!strcmp(var.value, "3032")) model = PETMODEL_3032;
      else if (!strcmp(var.value, "3032B")) model = PETMODEL_3032B;
      else if (!strcmp(var.value, "4016")) model = PETMODEL_4016;
      else if (!strcmp(var.value, "4032")) model = PETMODEL_4032;
      else if (!strcmp(var.value, "4032B")) model = PETMODEL_4032B;
      else if (!strcmp(var.value, "8032")) model = PETMODEL_8032;
      else if (!strcmp(var.value, "8096")) model = PETMODEL_8096;
      else if (!strcmp(var.value, "8296")) model = PETMODEL_8296;
      else if (!strcmp(var.value, "SUPERPET")) model = PETMODEL_SUPERPET;
      
      if (retro_ui_finalized && core_opt.Model != model)
      {
         petmodel_set(model);
         // Keyboard layout refresh required. All models below 8032 except B models use graphics layout, others use business.
         keyboard_init();
      }
      core_opt.Model = model;
   }
#elif defined(__XCBM2__)
   var.key = "vice_cbm2_model";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      int model = 0;

      if (!strcmp(var.value, "610 PAL")) model = CBM2MODEL_610_PAL;
      else if (!strcmp(var.value, "610 NTSC")) model = CBM2MODEL_610_NTSC;
      else if (!strcmp(var.value, "620 PAL")) model = CBM2MODEL_620_PAL;
      else if (!strcmp(var.value, "620 NTSC")) model = CBM2MODEL_620_NTSC;
      //else if (!strcmp(var.value, "620PLUS PAL")) model = CBM2MODEL_620PLUS_PAL;
      //else if (!strcmp(var.value, "620PLUS NTSC")) model = CBM2MODEL_620PLUS_NTSC;
      else if (!strcmp(var.value, "710 NTSC")) model = CBM2MODEL_710_NTSC;
      else if (!strcmp(var.value, "720 NTSC")) model = CBM2MODEL_720_NTSC;
      else if (!strcmp(var.value, "720PLUS NTSC")) model = CBM2MODEL_720PLUS_NTSC;

      if (retro_ui_finalized && core_opt.Model != model)
         cbm2model_set(model);

      core_opt.Model = model;
   }
#elif defined(__XCBM5x0__)
   var.key = "vice_cbm5x0_model";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      int model = 0;

      if (!strcmp(var.value, "510 PAL")) model = CBM2MODEL_510_PAL;
      else if (!strcmp(var.value, "510 NTSC")) model = CBM2MODEL_510_NTSC;

      if (retro_ui_finalized && core_opt.Model != model)
         cbm2model_set(model);

      core_opt.Model = model;
   }
#else
   var.key = "vice_c64_model";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      int model = 0;

      if (strstr(var.value, "auto")) opt_model_auto = 1;
      else opt_model_auto = 0;

      if (!strcmp(var.value, "C64 PAL auto")) model = C64MODEL_C64_PAL;
      else if (!strcmp(var.value, "C64 NTSC auto")) model = C64MODEL_C64_NTSC;
      else if (!strcmp(var.value, "C64C PAL auto")) model = C64MODEL_C64C_PAL;
      else if (!strcmp(var.value, "C64C NTSC auto")) model = C64MODEL_C64C_NTSC;
      else if (!strcmp(var.value, "C64 PAL")) model = C64MODEL_C64_PAL;
      else if (!strcmp(var.value, "C64 NTSC")) model = C64MODEL_C64_NTSC;
      else if (!strcmp(var.value, "C64C PAL")) model = C64MODEL_C64C_PAL;
      else if (!strcmp(var.value, "C64C NTSC")) model = C64MODEL_C64C_NTSC;
      else if (!strcmp(var.value, "C64SX PAL")) model = C64MODEL_C64SX_PAL;
      else if (!strcmp(var.value, "C64SX NTSC")) model = C64MODEL_C64SX_NTSC;
      else if (!strcmp(var.value, "PET64 PAL")) model = C64MODEL_PET64_PAL;
      else if (!strcmp(var.value, "PET64 NTSC")) model = C64MODEL_PET64_NTSC;
      else if (!strcmp(var.value, "C64 GS PAL")) model = C64MODEL_C64_GS;
      else if (!strcmp(var.value, "C64 JAP NTSC")) model = C64MODEL_C64_JAP;
      //else if (!strcmp(var.value, "C64 PAL N")) model = C64MODEL_C64_PAL_N;
      //else if (!strcmp(var.value, "C64 OLD PAL")) model = C64MODEL_C64_OLD_PAL;
      //else if (!strcmp(var.value, "C64 OLD NTSC")) model = C64MODEL_C64_OLD_NTSC;

      if (retro_ui_finalized && core_opt.Model != model)
      {
         c64model_set(model);
         request_model_prev = -1;
      }

      core_opt.Model = model;
   }
#endif

#if !defined(__XPET__) && !defined(__XPLUS4__) && !defined(__XVIC__)
   var.key = "vice_sid_engine";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      int sid_engine = 0;

      if (!strcmp(var.value, "ReSID")) sid_engine = 1;
      else if (!strcmp(var.value, "ReSID-3.3")) sid_engine = 6;
      else if (!strcmp(var.value, "ReSID-FP")) sid_engine = 7;

      if (retro_ui_finalized && core_opt.SidEngine != sid_engine)
      {
         if (core_opt.SidModel == 0xff)
            resources_set_int("SidEngine", sid_engine);
         else
            sid_set_engine_model(sid_engine, core_opt.SidModel);
      }

      core_opt.SidEngine = sid_engine;
   }

   var.key = "vice_sid_model";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      int sid_model = 0xff;

      if (!strcmp(var.value, "6581")) sid_model = 0;
      else if (!strcmp(var.value, "8580")) sid_model = 1;
      /* There is no digiboost for FastSID (and it's not needed either) */
      else if (!strcmp(var.value, "8580RD")) sid_model = (!core_opt.SidEngine ? 1 : 2);

      if (retro_ui_finalized && sid_model != 0xff)
         sid_set_engine_model(core_opt.SidEngine, sid_model);

      core_opt.SidModel = sid_model;
   }

   var.key = "vice_sid_extra";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      int sid_extra = atoi(var.value);
      if (strcmp(var.value, "disabled"))
         sid_extra = strtol(var.value, NULL, 16);

      if (retro_ui_finalized && core_opt.SidExtra != sid_extra)
      {
         if (!sid_extra)
            log_resources_set_int("SidStereo", 0);
         else
         {
            if (!core_opt.SidExtra)
               log_resources_set_int("SidStereo", 1);
            log_resources_set_int("SidStereoAddressStart", sid_extra);
         }
      }

      core_opt.SidExtra = sid_extra;
   }

   var.key = "vice_resid_sampling";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      int val = 0;

      if (!strcmp(var.value, "Fast")) val = 0;
      else if (!strcmp(var.value, "Interpolation")) val = 1;
      else if (!strcmp(var.value, "Resampling")) val = 2;
      else if (!strcmp(var.value, "Fast resampling")) val = 3;

      if (retro_ui_finalized && core_opt.SidResidSampling != val)
         log_resources_set_int("SidResidSampling", val);

      core_opt.SidResidSampling = val;
   }

   var.key = "vice_resid_passband";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      int val = atoi(var.value);

      if (retro_ui_finalized && core_opt.SidResidPassband != val)
      {
         log_resources_set_int("SidResidPassband", val);
         log_resources_set_int("SidResid8580Passband", val);
      }

      core_opt.SidResidPassband = val;
   }

   var.key = "vice_resid_gain";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      int val = atoi(var.value);

      if (retro_ui_finalized && core_opt.SidResidGain != val)
      {
         log_resources_set_int("SidResidGain", val);
         log_resources_set_int("SidResid8580Gain", val);
      }

      core_opt.SidResidGain = val;
   }

   var.key = "vice_resid_filterbias";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      int val = atoi(var.value);

      if (retro_ui_finalized && core_opt.SidResidFilterBias != val)
         log_resources_set_int("SidResidFilterBias", val);

      core_opt.SidResidFilterBias = val;
   }

   var.key = "vice_resid_8580filterbias";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      int val = atoi(var.value);

      if (retro_ui_finalized && core_opt.SidResid8580FilterBias != val)
         log_resources_set_int("SidResid8580FilterBias", val);

      core_opt.SidResid8580FilterBias = val;
   }
#endif

#if defined(__X64__) || defined(__X64SC__) || defined(__X128__) || defined(__XSCPU64__) || defined(__XCBM5x0__) || defined(__XVIC__) || defined(__XPLUS4__)
   var.key = "vice_zoom_mode";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (!strcmp(var.value, "none")) zoom_mode_id = 0;
      else if (!strcmp(var.value, "small")) zoom_mode_id = 1;
      else if (!strcmp(var.value, "medium")) zoom_mode_id = 2;
      else if (!strcmp(var.value, "maximum")) zoom_mode_id = 3;
      else if (!strcmp(var.value, "manual")) zoom_mode_id = 4;

#if defined(__X128__)
      if (core_opt.C128ColumnKey == 0)
         zoom_mode_id = 0;
#endif

      opt_zoom_mode_id = zoom_mode_id;
   }

   var.key = "vice_zoom_mode_crop";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      int zoom_mode_crop_id_prev = zoom_mode_crop_id;

      if (!strcmp(var.value, "both")) zoom_mode_crop_id = 0;
      else if (!strcmp(var.value, "vertical")) zoom_mode_crop_id = 1;
      else if (!strcmp(var.value, "horizontal")) zoom_mode_crop_id = 2;
      else if (!strcmp(var.value, "16:9")) zoom_mode_crop_id = 3;
      else if (!strcmp(var.value, "16:10")) zoom_mode_crop_id = 4;
      else if (!strcmp(var.value, "4:3")) zoom_mode_crop_id = 5;
      else if (!strcmp(var.value, "5:4")) zoom_mode_crop_id = 6;

      // Zoom reset
      if (zoom_mode_crop_id != zoom_mode_crop_id_prev)
         zoom_mode_id_prev = -1;
   }

   var.key = "vice_aspect_ratio";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      int opt_aspect_ratio_prev = opt_aspect_ratio;

      if (!strcmp(var.value, "auto")) opt_aspect_ratio = 0;
      else if (!strcmp(var.value, "pal")) opt_aspect_ratio = 1;
      else if (!strcmp(var.value, "ntsc")) opt_aspect_ratio = 2;
      else if (!strcmp(var.value, "raw")) opt_aspect_ratio = 3;

      // Zoom reset
      if (opt_aspect_ratio != opt_aspect_ratio_prev)
         zoom_mode_id_prev = -1;
   }

   var.key = "vice_manual_crop_top";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      int manual_crop_top_prev = manual_crop_top;
      manual_crop_top = atoi(var.value);
      if (manual_crop_top != manual_crop_top_prev)
         zoom_mode_id_prev = -1;
   }
   var.key = "vice_manual_crop_bottom";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      int manual_crop_bottom_prev = manual_crop_bottom;
      manual_crop_bottom = atoi(var.value);
      if (manual_crop_bottom != manual_crop_bottom_prev)
         zoom_mode_id_prev = -1;
   }
   var.key = "vice_manual_crop_left";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      int manual_crop_left_prev = manual_crop_left;
      manual_crop_left = atoi(var.value);
      if (manual_crop_left != manual_crop_left_prev)
         zoom_mode_id_prev = -1;
   }
   var.key = "vice_manual_crop_right";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      int manual_crop_right_prev = manual_crop_right;
      manual_crop_right = atoi(var.value);
      if (manual_crop_right != manual_crop_right_prev)
         zoom_mode_id_prev = -1;
   }
#endif

   var.key = "vice_gfx_colors";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      // Only allow screenmode change after restart
      if (!pix_bytes_initialized)
      {
         if (!strcmp(var.value, "16bit")) pix_bytes = 2;
         else if (!strcmp(var.value, "24bit")) pix_bytes = 4;
         pix_bytes_initialized = true;
      }
   }

#if defined(__XVIC__)
   var.key = "vice_vic20_external_palette";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (retro_ui_finalized && strcmp(var.value, core_opt.ExternalPalette))
      {
         if (!strcmp(var.value, "default"))
            log_resources_set_int("VICExternalPalette", 0);
         else
         {
            log_resources_set_int("VICExternalPalette", 1);
            log_resources_set_string("VICPaletteFile", var.value);
         }
      }

      sprintf(core_opt.ExternalPalette, "%s", var.value);
   }
#elif defined(__XPLUS4__)
   var.key = "vice_plus4_external_palette";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (retro_ui_finalized && strcmp(var.value, core_opt.ExternalPalette))
      {
         if (!strcmp(var.value, "default"))
            log_resources_set_int("TEDExternalPalette", 0);
         else
         {
            log_resources_set_int("TEDExternalPalette", 1);
            log_resources_set_string("TEDPaletteFile", var.value);
         }
      }

      sprintf(core_opt.ExternalPalette, "%s", var.value);
   }
#elif defined(__XPET__)
   var.key = "vice_pet_external_palette";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (retro_ui_finalized && strcmp(var.value, core_opt.ExternalPalette))
      {
         if (!strcmp(var.value, "default"))
            log_resources_set_int("CrtcExternalPalette", 0);
         else
         {
            log_resources_set_int("CrtcExternalPalette", 1);
            log_resources_set_string("CrtcPaletteFile", var.value);
         }
      }

      sprintf(core_opt.ExternalPalette, "%s", var.value);
   }
#elif defined(__XCBM2__)
   var.key = "vice_cbm2_external_palette";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (retro_ui_finalized && strcmp(var.value, core_opt.ExternalPalette))
      {
         if (!strcmp(var.value, "default"))
            log_resources_set_int("CrtcExternalPalette", 0);
         else
         {
            log_resources_set_int("CrtcExternalPalette", 1);
            log_resources_set_string("CrtcPaletteFile", var.value);
         }
      }

      sprintf(core_opt.ExternalPalette, "%s", var.value);
   }
#else
   var.key = "vice_external_palette";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (retro_ui_finalized && strcmp(var.value, core_opt.ExternalPalette))
      {
         if (!strcmp(var.value, "default"))
            log_resources_set_int("VICIIExternalPalette", 0);
         else
         {
            log_resources_set_int("VICIIExternalPalette", 1);
            log_resources_set_string("VICIIPaletteFile", var.value);
         }
      }

      sprintf(core_opt.ExternalPalette, "%s", var.value);
   }
#endif

#if defined(__X64__) || defined(__X64SC__) || defined(__X128__) || defined(__XSCPU64__) || defined(__XCBM5x0__) || defined(__XVIC__) || defined(__XPLUS4__)
#if defined(__X64__) || defined(__X64SC__) || defined(__X128__) || defined(__XSCPU64__) || defined(__XCBM5x0__)
   var.key = "vice_vicii_color_gamma";
#elif defined(__XVIC__)
   var.key = "vice_vic_color_gamma";
#elif defined(__XPLUS4__)
   var.key = "vice_ted_color_gamma";
#endif
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      int color_gamma = atoi(var.value);

      if (retro_ui_finalized && core_opt.ColorGamma != color_gamma)
#if defined(__X64__) || defined(__X64SC__) || defined(__X128__) || defined(__XSCPU64__) || defined(__XCBM5x0__)
         log_resources_set_int("VICIIColorGamma", color_gamma);
#elif defined(__XVIC__)
         log_resources_set_int("VICColorGamma", color_gamma);
#elif defined(__XPLUS4__)
         log_resources_set_int("TEDColorGamma", color_gamma);
#endif

      core_opt.ColorGamma = color_gamma;
   }
#if defined(__X64__) || defined(__X64SC__) || defined(__X128__) || defined(__XSCPU64__) || defined(__XCBM5x0__)
   var.key = "vice_vicii_color_saturation";
#elif defined(__XVIC__)
   var.key = "vice_vic_color_saturation";
#elif defined(__XPLUS4__)
   var.key = "vice_ted_color_saturation";
#endif
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      int color_saturation = atoi(var.value);

      if (retro_ui_finalized && core_opt.ColorSaturation != color_saturation)
#if defined(__X64__) || defined(__X64SC__) || defined(__X128__) || defined(__XSCPU64__) || defined(__XCBM5x0__)
         log_resources_set_int("VICIIColorSaturation", color_saturation);
#elif defined(__XVIC__)
         log_resources_set_int("VICColorSaturation", color_saturation);
#elif defined(__XPLUS4__)
         log_resources_set_int("TEDColorSaturation", color_saturation);
#endif

      core_opt.ColorSaturation = color_saturation;
   }

#if defined(__X64__) || defined(__X64SC__) || defined(__X128__) || defined(__XSCPU64__) || defined(__XCBM5x0__)
   var.key = "vice_vicii_color_contrast";
#elif defined(__XVIC__)
   var.key = "vice_vic_color_contrast";
#elif defined(__XPLUS4__)
   var.key = "vice_ted_color_contrast";
#endif
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      int color_contrast = atoi(var.value);

      if (retro_ui_finalized && core_opt.ColorContrast != color_contrast)
#if defined(__X64__) || defined(__X64SC__) || defined(__X128__) || defined(__XSCPU64__) || defined(__XCBM5x0__)
         log_resources_set_int("VICIIColorContrast", color_contrast);
#elif defined(__XVIC__)
         log_resources_set_int("VICColorContrast", color_contrast);
#elif defined(__XPLUS4__)
         log_resources_set_int("TEDColorContrast", color_contrast);
#endif

      core_opt.ColorContrast = color_contrast;
   }

#if defined(__X64__) || defined(__X64SC__) || defined(__X128__) || defined(__XSCPU64__) || defined(__XCBM5x0__)
   var.key = "vice_vicii_color_brightness";
#elif defined(__XVIC__)
   var.key = "vice_vic_color_brightness";
#elif defined(__XPLUS4__)
   var.key = "vice_ted_color_brightness";
#endif
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      int color_brightness = atoi(var.value);

      if (retro_ui_finalized && core_opt.ColorBrightness != color_brightness)
#if defined(__X64__) || defined(__X64SC__) || defined(__X128__) || defined(__XSCPU64__) || defined(__XCBM5x0__)
         log_resources_set_int("VICIIColorBrightness", color_brightness);
#elif defined(__XVIC__)
         log_resources_set_int("VICColorBrightness", color_brightness);
#elif defined(__XPLUS4__)
         log_resources_set_int("TEDColorBrightness", color_brightness);
#endif

      core_opt.ColorBrightness = color_brightness;
   }
#endif

#if !defined(__XCBM5x0__)
   var.key = "vice_userport_joytype";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      int joyadaptertype = -1;

      if (!strcmp(var.value, "None")) joyadaptertype = -1;
      else if (!strcmp(var.value, "Protovision CGA")) joyadaptertype = USERPORT_JOYSTICK_CGA;
      else if (!strcmp(var.value, "PET")) joyadaptertype = USERPORT_JOYSTICK_PET;
      else if (!strcmp(var.value, "Hummer")) joyadaptertype = USERPORT_JOYSTICK_HUMMER;
      else if (!strcmp(var.value, "OEM")) joyadaptertype = USERPORT_JOYSTICK_OEM;
      else if (!strcmp(var.value, "Hit")) joyadaptertype = USERPORT_JOYSTICK_HIT;
      else if (!strcmp(var.value, "Kingsoft")) joyadaptertype = USERPORT_JOYSTICK_KINGSOFT;
      else if (!strcmp(var.value, "Starbyte")) joyadaptertype = USERPORT_JOYSTICK_STARBYTE;

      if (retro_ui_finalized && core_opt.UserportJoy != joyadaptertype)
      {
         if (joyadaptertype == -1)
            log_resources_set_int("UserportJoy", 0);
         else
         {
            log_resources_set_int("UserportJoy", 1);
            log_resources_set_int("UserportJoyType", joyadaptertype);
         }
      }

      core_opt.UserportJoy = joyadaptertype;
   }
#endif

#if !defined(__XPET__) && !defined(__XCBM2__) && !defined(__XCBM5x0__) && !defined(__XVIC__)
   var.key = "vice_joyport";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (!strcmp(var.value, "Port 2") && !cur_port_locked) cur_port = 2;
      else if (!strcmp(var.value, "Port 1") && !cur_port_locked) cur_port = 1;
   }

   var.key = "vice_joyport_type";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      opt_joyport_type = atoi(var.value);
   }

   var.key = "vice_analogmouse_deadzone";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      opt_analogmouse_deadzone = atoi(var.value);
   }

   var.key = "vice_analogmouse_speed";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      opt_analogmouse_speed = atof(var.value);
   }

   var.key = "vice_dpadmouse_speed";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      opt_dpadmouse_speed = atoi(var.value);
   }

   var.key = "vice_mouse_speed";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      opt_mouse_speed = atoi(var.value);
   }
#endif

   var.key = "vice_keyrah_keypad_mappings";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (!strcmp(var.value, "disabled")) opt_keyrah_keypad = 0;
      else if (!strcmp(var.value, "enabled")) opt_keyrah_keypad = 1;
   }

   var.key = "vice_keyboard_keymap";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      int val = opt_keyboard_keymap;

      if (!strcmp(var.value, "symbolic")) opt_keyboard_keymap = KBD_INDEX_SYM;
      else if (!strcmp(var.value, "positional")) opt_keyboard_keymap = KBD_INDEX_POS;
      else if (!strcmp(var.value, "symbolic-user")) opt_keyboard_keymap = KBD_INDEX_USERSYM;
      else if (!strcmp(var.value, "positional-user")) opt_keyboard_keymap = KBD_INDEX_USERPOS;

      if (retro_ui_finalized && opt_keyboard_keymap != val)
         keyboard_init();
   }

   var.key = "vice_physical_keyboard_pass_through";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (!strcmp(var.value, "disabled")) opt_keyboard_passthrough = 0;
      else if (!strcmp(var.value, "enabled")) opt_keyboard_passthrough = 1;
   }

   var.key = "vice_retropad_options";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (!strcmp(var.value, "disabled")) opt_retropad_options = 0;
      else if (!strcmp(var.value, "rotate")) opt_retropad_options = 1;
      else if (!strcmp(var.value, "jump")) opt_retropad_options = 2;
      else if (!strcmp(var.value, "rotate_jump")) opt_retropad_options = 3;
   }

   var.key = "vice_turbo_fire_button";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (!strcmp(var.value, "disabled")) turbo_fire_button = -1;
      else if (!strcmp(var.value, "A")) turbo_fire_button = RETRO_DEVICE_ID_JOYPAD_A;
      else if (!strcmp(var.value, "Y")) turbo_fire_button = RETRO_DEVICE_ID_JOYPAD_Y;
      else if (!strcmp(var.value, "X")) turbo_fire_button = RETRO_DEVICE_ID_JOYPAD_X;
      else if (!strcmp(var.value, "L")) turbo_fire_button = RETRO_DEVICE_ID_JOYPAD_L;
      else if (!strcmp(var.value, "R")) turbo_fire_button = RETRO_DEVICE_ID_JOYPAD_R;
      else if (!strcmp(var.value, "L2")) turbo_fire_button = RETRO_DEVICE_ID_JOYPAD_L2;
      else if (!strcmp(var.value, "R2")) turbo_fire_button = RETRO_DEVICE_ID_JOYPAD_R2;
      else if (!strcmp(var.value, "L3")) turbo_fire_button = RETRO_DEVICE_ID_JOYPAD_L3;
      else if (!strcmp(var.value, "R3")) turbo_fire_button = RETRO_DEVICE_ID_JOYPAD_R3;
   }

   var.key = "vice_turbo_pulse";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      turbo_pulse = atoi(var.value);
   }

   var.key = "vice_reset";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (!strcmp(var.value, "Autostart")) opt_reset_type = 0;
      else if (!strcmp(var.value, "Soft")) opt_reset_type = 1;
      else if (!strcmp(var.value, "Hard")) opt_reset_type = 2;
      else if (!strcmp(var.value, "Freeze")) opt_reset_type = 3;
   }

   var.key = "vice_vkbd_theme";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      opt_vkbd_theme = atoi(var.value);
   }

   var.key = "vice_vkbd_alpha";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      opt_vkbd_alpha = 255 - (255 * atoi(var.value) / 100);
      vkbd_alpha = opt_vkbd_alpha;
   }

   var.key = "vice_statusbar";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      opt_statusbar = 0;

      if (strstr(var.value, "top")) opt_statusbar |= STATUSBAR_TOP;
      else opt_statusbar |= STATUSBAR_BOTTOM;

      if (strstr(var.value, "basic")) opt_statusbar |= STATUSBAR_BASIC;
      if (strstr(var.value, "minimal")) opt_statusbar |= STATUSBAR_MINIMAL;
   }

   var.key = "vice_mapping_options_display";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (!strcmp(var.value, "disabled")) opt_mapping_options_display = 0;
      else if (!strcmp(var.value, "enabled")) opt_mapping_options_display = 1;
   }

   var.key = "vice_audio_options_display";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (!strcmp(var.value, "disabled")) opt_audio_options_display = 0;
      else if (!strcmp(var.value, "enabled")) opt_audio_options_display = 1;
   }

   var.key = "vice_video_options_display";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (!strcmp(var.value, "disabled")) opt_video_options_display = 0;
      else if (!strcmp(var.value, "enabled")) opt_video_options_display = 1;
   }

   var.key = "vice_read_vicerc";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (!strcmp(var.value, "disabled")) opt_read_vicerc = 0;
      else if (!strcmp(var.value, "enabled")) opt_read_vicerc = 1;

      request_reload_restart = (opt_read_vicerc != opt_read_vicerc_prev) ? 1 : request_reload_restart;
      opt_read_vicerc_prev = opt_read_vicerc;
   }

#if defined(__X64__) || defined(__X64SC__) || defined(__X128__) || defined(__XSCPU64__)
   var.key = "vice_jiffydos";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (!strcmp(var.value, "disabled")) opt_jiffydos = 0;
      else if (!strcmp(var.value, "enabled")) opt_jiffydos = 1;

      if (!opt_jiffydos_allow)
         opt_jiffydos = 0;

      request_reload_restart = (opt_jiffydos != opt_jiffydos_prev) ? 1 : request_reload_restart;
      opt_jiffydos_prev = opt_jiffydos;
   }
#endif

   /* Mapper */
   var.key = "vice_mapper_select";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      mapper_keys[RETRO_DEVICE_ID_JOYPAD_SELECT] = keyId(var.value);
   }

   var.key = "vice_mapper_start";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      mapper_keys[RETRO_DEVICE_ID_JOYPAD_START] = keyId(var.value);
   }

   var.key = "vice_mapper_b";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      mapper_keys[RETRO_DEVICE_ID_JOYPAD_B] = keyId(var.value);
   }

   var.key = "vice_mapper_a";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      mapper_keys[RETRO_DEVICE_ID_JOYPAD_A] = keyId(var.value);
   }

   var.key = "vice_mapper_y";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      mapper_keys[RETRO_DEVICE_ID_JOYPAD_Y] = keyId(var.value);
   }

   var.key = "vice_mapper_x";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      mapper_keys[RETRO_DEVICE_ID_JOYPAD_X] = keyId(var.value);
   }

   var.key = "vice_mapper_l";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      mapper_keys[RETRO_DEVICE_ID_JOYPAD_L] = keyId(var.value);
   }

   var.key = "vice_mapper_r";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      mapper_keys[RETRO_DEVICE_ID_JOYPAD_R] = keyId(var.value);
   }

   var.key = "vice_mapper_l2";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      mapper_keys[RETRO_DEVICE_ID_JOYPAD_L2] = keyId(var.value);
   }

   var.key = "vice_mapper_r2";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      mapper_keys[RETRO_DEVICE_ID_JOYPAD_R2] = keyId(var.value);
   }

   var.key = "vice_mapper_l3";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      mapper_keys[RETRO_DEVICE_ID_JOYPAD_L3] = keyId(var.value);
   }

   var.key = "vice_mapper_r3";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      mapper_keys[RETRO_DEVICE_ID_JOYPAD_R3] = keyId(var.value);
   }



   var.key = "vice_mapper_lr";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      mapper_keys[16] = keyId(var.value);
   }

   var.key = "vice_mapper_ll";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      mapper_keys[17] = keyId(var.value);
   }

   var.key = "vice_mapper_ld";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      mapper_keys[18] = keyId(var.value);
   }

   var.key = "vice_mapper_lu";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      mapper_keys[19] = keyId(var.value);
   }

   var.key = "vice_mapper_rr";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      mapper_keys[20] = keyId(var.value);
   }

   var.key = "vice_mapper_rl";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      mapper_keys[21] = keyId(var.value);
   }

   var.key = "vice_mapper_rd";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      mapper_keys[22] = keyId(var.value);
   }

   var.key = "vice_mapper_ru";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      mapper_keys[23] = keyId(var.value);
   }


   var.key = "vice_mapper_vkbd";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      mapper_keys[24] = keyId(var.value);
   }

   var.key = "vice_mapper_statusbar";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      mapper_keys[25] = keyId(var.value);
   }

   var.key = "vice_mapper_joyport_switch";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      mapper_keys[26] = keyId(var.value);
   }

   var.key = "vice_mapper_reset";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      mapper_keys[27] = keyId(var.value);
   }

   var.key = "vice_mapper_zoom_mode_toggle";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      mapper_keys[28] = keyId(var.value);
   }

   var.key = "vice_mapper_warp_mode";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      mapper_keys[29] = keyId(var.value);
   }

   var.key = "vice_datasette_hotkeys";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "enabled") == 0) datasette_hotkeys=1;
      else if (strcmp(var.value, "disabled") == 0) datasette_hotkeys=0;
   }

   var.key = "vice_mapper_datasette_toggle_hotkeys";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      mapper_keys[30] = keyId(var.value);
   }
   
   var.key = "vice_mapper_datasette_stop";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      mapper_keys[31] = keyId(var.value);
   }

   var.key = "vice_mapper_datasette_start";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      mapper_keys[32] = keyId(var.value);
   }

   var.key = "vice_mapper_datasette_forward";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      mapper_keys[33] = keyId(var.value);
   }

   var.key = "vice_mapper_datasette_rewind";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      mapper_keys[34] = keyId(var.value);
   }

   var.key = "vice_mapper_datasette_reset";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      mapper_keys[35] = keyId(var.value);
   }


   /*** Options display ***/

   /* Mapping options */
   option_display.visible = opt_mapping_options_display;

   option_display.key = "vice_mapper_select";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_mapper_start";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_mapper_b";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_mapper_a";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_mapper_y";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_mapper_x";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_mapper_l";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_mapper_r";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_mapper_l2";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_mapper_r2";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_mapper_l3";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_mapper_r3";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_mapper_lu";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_mapper_ld";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_mapper_ll";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_mapper_lr";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_mapper_ru";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_mapper_rd";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_mapper_rl";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_mapper_rr";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_mapper_vkbd";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_mapper_statusbar";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
#if !defined(__XPET__) && !defined(__XCBM2__) && !defined(__XCBM5x0__) && !defined(__XVIC__)
   option_display.key = "vice_mapper_joyport_switch";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
#endif
#if defined(__X64__) || defined(__X64SC__) || defined(__X128__) || defined(__XSCPU64__) || defined(__XCBM5x0__) || defined(__XVIC__) || defined(__XPLUS4__)
   option_display.key = "vice_mapper_zoom_mode_toggle";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
#endif
   option_display.key = "vice_mapper_reset";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_mapper_warp_mode";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_mapper_datasette_toggle_hotkeys";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_mapper_datasette_start";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_mapper_datasette_stop";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_mapper_datasette_rewind";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_mapper_datasette_forward";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_mapper_datasette_reset";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);

   /* Audio options */
   option_display.visible = opt_audio_options_display;

   option_display.key = "vice_drive_sound_emulation";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
#if defined(__X64__) || defined(__X64SC__) || defined(__X128__) || defined(__XSCPU64__) || defined(__XCBM5x0__) || defined(__XVIC__) || defined(__XPLUS4__)
   option_display.key = "vice_audio_leak_emulation";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
#endif
   option_display.key = "vice_sound_sample_rate";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
#if !defined(__XPET__) && !defined(__XPLUS4__) && !defined(__XVIC__)
   option_display.key = "vice_sid_engine";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_sid_model";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_sid_extra";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_resid_sampling";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_resid_passband";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_resid_gain";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_resid_filterbias";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_resid_8580filterbias";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
#endif

   /* Video options */
   option_display.visible = opt_video_options_display;

#if defined(__X64__) || defined(__X64SC__) || defined(__X128__) || defined(__XSCPU64__) || defined(__XCBM5x0__) || defined(__XVIC__) || defined(__XPLUS4__)
   option_display.key = "vice_zoom_mode";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_zoom_mode_crop";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_aspect_ratio";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_manual_crop_top";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_manual_crop_bottom";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_manual_crop_left";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_manual_crop_right";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
#endif
   option_display.key = "vice_gfx_colors";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
#if defined(__XVIC__)
   option_display.key = "vice_vic20_external_palette";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
#elif defined(__XPLUS4__)
   option_display.key = "vice_plus4_external_palette";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
#elif defined(__XPET__)
   option_display.key = "vice_pet_external_palette";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
#elif defined(__XCBM2__)
   option_display.key = "vice_cbm2_external_palette";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
#else
   option_display.key = "vice_external_palette";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_vicii_color_gamma",
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_vicii_color_saturation",
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_vicii_color_contrast",
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_vicii_color_brightness",
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
#endif
   option_display.key = "vice_vkbd_theme";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_vkbd_alpha";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   option_display.key = "vice_statusbar";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
}

void emu_reset(int type)
{
   // Always stop datasette or autostart from tape will fail
   datasette_control(DATASETTE_CONTROL_STOP);

   // Always disable Warp
   resources_set_int("WarpMode", 0);

   // Changing opt_read_vicerc requires reloading
   if (request_reload_restart)
      reload_restart();

   // Follow core option type with -1
   type = (type == -1) ? opt_reset_type : type;
   switch (type)
   {
      case 0:
         // Hard reset before autostart
         machine_trigger_reset(MACHINE_RESET_MODE_HARD);

         // Allow autostarting with a different disk
         if (dc->count > 1)
            autostartString = x_strdup(dc->files[dc->index]);
         if (autostartString != NULL && autostartString[0] != '\0' && !noautostart)
            autostart_autodetect(autostartString, NULL, 0, AUTOSTART_MODE_RUN);
         break;
      case 1:
         machine_trigger_reset(MACHINE_RESET_MODE_SOFT);
         break;
      case 2:
         machine_trigger_reset(MACHINE_RESET_MODE_HARD);
         break;
      case 3:
         cartridge_trigger_freeze();
         break;
   }
}

void retro_reset(void)
{
   // Always stop datasette, or autostart from tape will fail
   datasette_control(DATASETTE_CONTROL_STOP);

   // Always disable Warp
   resources_set_int("WarpMode", 0);

   // Changing opt_read_vicerc requires reloading
   if (request_reload_restart)
      reload_restart();

   // Retro reset should always hard reset & autostart
   machine_trigger_reset(MACHINE_RESET_MODE_HARD);

   // Allow autostarting with a different disk
   if (dc->count > 1)
      autostartString = x_strdup(dc->files[dc->index]);
   if (autostartString != NULL && autostartString[0] != '\0' && !noautostart)
      autostart_autodetect(autostartString, NULL, 0, AUTOSTART_MODE_RUN);
}

struct DiskImage {
    char* fname;
};

static bool retro_set_eject_state(bool ejected)
{
    if (dc)
    {
        int unit = get_image_unit();

        if (dc->eject_state == ejected)
            return true;

        if (ejected && dc->index <= dc->count && dc->files[dc->index] != NULL)
        {
            if (unit == 1)
                tape_image_detach(unit);
            else if (unit >= 8 && unit <= 11)
                file_system_detach_disk(unit);
            else if (unit == 0)
                cartridge_detach_image(-1);
            display_current_image("", false);
        }
        else if (!ejected && dc->index < dc->count && dc->files[dc->index] != NULL)
        {
            if (unit == 1)
                tape_image_attach(unit, dc->files[dc->index]);
            else if (unit >= 8 && unit <= 11)
            {
                file_system_attach_disk(unit, dc->files[dc->index]);
                autodetect_drivetype(unit);
            }
            else if (unit == 0)
            {
#if defined(__XVIC__)
                cartridge_attach_image(CARTRIDGE_VIC20_DETECT, dc->files[dc->index]);
#elif defined(__XPLUS4__)
                cartridge_attach_image(CARTRIDGE_PLUS4_DETECT, dc->files[dc->index]);
                // Soft reset required, otherwise gfx gets corrupted (?!)
                emu_reset(1);
#else
                cartridge_attach_image(0, dc->files[dc->index]);
#endif
                // PRGs must autostart on attach, cartridges reset anyway
                if (strendswith(dc->files[dc->index], "prg"))
                    emu_reset(0);
            }
            display_current_image(dc->files[dc->index], true);
        }

        dc->eject_state = ejected;
        return true;
    }

    return false;
}

/* Gets current eject state. The initial state is 'not ejected'. */
bool retro_get_eject_state(void)
{
    if (dc)
        return dc->eject_state;

    return true;
}

/* Gets current disk index. First disk is index 0.
 * If return value is >= get_num_images(), no disk is currently inserted.
 */
static unsigned retro_get_image_index(void)
{
    if (dc)
        return dc->index;

    return 0;
}

/* Sets image index. Can only be called when disk is ejected.
 * The implementation supports setting "no disk" by using an
 * index >= get_num_images().
 */
bool retro_set_image_index(unsigned index)
{
    // Switch disk in drive
    if (dc)
    {
        if (index <= dc->count)
        {
            dc->index = index;

            if ((index < dc->count) && (dc->files[index]))
            {
                log_disk_in_tray(display_disk_name);
                display_current_image(dc->files[dc->index], false);
            }
            return true;
        }
    }

    return false;
}

/* Gets total number of images which are available to use. */
static unsigned retro_get_num_images(void)
{
    if (dc)
        return dc->count;

    return 0;
}


/* Replaces the disk image associated with index.
 * Arguments to pass in info have same requirements as retro_load_game().
 * Virtual disk tray must be ejected when calling this.
 *
 * Replacing a disk image with info = NULL will remove the disk image
 * from the internal list.
 * As a result, calls to get_image_index() can change.
 *
 * E.g. replace_image_index(1, NULL), and previous get_image_index()
 * returned 4 before.
 * Index 1 will be removed, and the new index is 3.
 */
static bool retro_replace_image_index(unsigned index, const struct retro_game_info *info)
{
    if (dc)
    {
        if (info != NULL)
        {
            dc_replace_file(dc, index, info->path);
        }
        else
        {
            dc_remove_file(dc, index);
        }
        return true;
    }

    return false;	
}

/* Adds a new valid index (get_num_images()) to the internal disk list.
 * This will increment subsequent return values from get_num_images() by 1.
 * This image index cannot be used until a disk image has been set
 * with replace_image_index. */
static bool retro_add_image_index(void)
{
    if (dc)
    {
        if (dc->count <= DC_MAX_SIZE)
        {
            dc->files[dc->count] = NULL;
            dc->labels[dc->count] = NULL;
            dc->names[dc->count] = NULL;
            dc->count++;
            return true;
        }
    }

    return false;
}

static bool retro_get_image_path(unsigned index, char *path, size_t len)
{
    if (len < 1)
        return false;

    if (dc)
    {
        if (index < dc->count)
        {
            if (!string_is_empty(dc->files[index]))
            {
                strlcpy(path, dc->files[index], len);
                return true;
            }
        }
    }

   return false;
}

static bool retro_get_image_label(unsigned index, char *label, size_t len)
{
    if (len < 1)
        return false;

    if (dc)
    {
        if (index < dc->count)
        {
            if (!string_is_empty(dc->names[index]))
            {
                strlcpy(label, dc->names[index], len);
                return true;
            }
        }
    }

    return false;
}

static struct retro_disk_control_callback diskControl = {
    retro_set_eject_state,
    retro_get_eject_state,
    retro_get_image_index,
    retro_set_image_index,
    retro_get_num_images,
    retro_replace_image_index,
    retro_add_image_index,
};

static struct retro_disk_control_ext_callback diskControlExt = {
   retro_set_eject_state,
   retro_get_eject_state,
   retro_get_image_index,
   retro_set_image_index,
   retro_get_num_images,
   retro_replace_image_index,
   retro_add_image_index,
   NULL, // set_initial_image
   retro_get_image_path,
   retro_get_image_label,
};

static void fallback_log(enum retro_log_level level, const char *fmt, ...)
{
   (void)level;
   va_list va;
   va_start(va, fmt);
   vfprintf(stderr, fmt, va);
   va_end(va);
}

void retro_init(void)
{
   struct retro_log_callback log;

   // Init disk control context
   dc = dc_create();

   log_cb = fallback_log;
   if (environ_cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &log))
      log_cb = log.log;

   const char *system_dir = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY, &system_dir) && system_dir)
   {
      // if defined, use the system directory
      strlcpy(
            retro_system_directory,
            system_dir,
            sizeof(retro_system_directory));
   }

   const char *content_dir = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_CONTENT_DIRECTORY, &content_dir) && content_dir)
   {
      // if defined, use the system directory
      strlcpy(
            retro_content_directory,
            content_dir,
            sizeof(retro_content_directory));
   }

   const char *save_dir = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY, &save_dir) && save_dir)
   {
      // If save directory is defined use it, otherwise use system directory
      strlcpy(
            retro_save_directory,
            string_is_empty(save_dir) ? retro_system_directory : save_dir,
            sizeof(retro_save_directory));
   }
   else
   {
      // make retro_save_directory the same in case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY is not implemented by the frontend
      strlcpy(
            retro_save_directory,
            retro_system_directory,
            sizeof(retro_save_directory));
   }

   if (retro_system_directory==NULL)
   {
#if defined(__ANDROID__) || defined(ANDROID)
      strcpy(RETRO_DIR, "/mnt/sdcard");
#elif defined(VITA)
      strcpy(RETRO_DIR, "ux0:/data");
#elif defined(__SWITCH__)
      strcpy(RETRO_DIR, "/");
#else
      strcpy(RETRO_DIR, ".");
#endif
   }
   else
      sprintf(RETRO_DIR, "%s", retro_system_directory);

   /* Use system directory for data files such as C64/.vpl etc. */
   snprintf(retro_system_data_directory, sizeof(retro_system_data_directory), "%s%svice", RETRO_DIR, FSDEV_DIR_SEP_STR);
   int dir_mode = 0;
#if defined(IOS)
   dir_mode = 0755;
#elif defined(VITA) || defined(PSP)
   dir_mode = 0777;
#elif defined(PS2)
   dir_mode = 0777;
#elif defined(ORBIS)
   dir_mode = 0755;
#elif defined(__QNX__)
   dir_mode = 0777;
#else
   dir_mode = 0750;
#endif
   archdep_mkdir(retro_system_data_directory, dir_mode);

   enum retro_pixel_format fmt = RETRO_PIXEL_FORMAT_RGB565;
   if (!environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt))
   {
      log_cb(RETRO_LOG_ERROR, "PIXEL FORMAT RGB565 is not supported.\n");
      environ_cb(RETRO_ENVIRONMENT_SHUTDOWN, NULL);
      return;
   }

   memset(retro_bmp, 0, sizeof(retro_bmp));
   memset(core_key_state, 0, 512);
   memset(core_old_key_state, 0, sizeof(core_old_key_state));

#define RETRO_DESCRIPTOR_BLOCK( _user )                                            \
   { _user, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A, "A" },               \
   { _user, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B, "B / Fire" },        \
   { _user, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X, "X" },               \
   { _user, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y, "Y" },               \
   { _user, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT, "Select" },     \
   { _user, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START, "Start" },       \
   { _user, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "Right" },       \
   { _user, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT, "Left" },         \
   { _user, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP, "Up" },             \
   { _user, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN, "Down" },         \
   { _user, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R, "R" },               \
   { _user, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L, "L" },               \
   { _user, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2, "R2" },             \
   { _user, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2, "L2" },             \
   { _user, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R3, "R3" },             \
   { _user, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L3, "L3" },			   \
   { _user, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X, "Left Analog X" },			   \
   { _user, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y, "Left Analog Y" },			   \
   { _user, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_X, "Right Analog X" },			   \
   { _user, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_Y, "Right Analog Y" }
   
   static struct retro_input_descriptor inputDescriptors[] =
   {
      RETRO_DESCRIPTOR_BLOCK( 0 ),
      RETRO_DESCRIPTOR_BLOCK( 1 ),
      RETRO_DESCRIPTOR_BLOCK( 2 ),
      RETRO_DESCRIPTOR_BLOCK( 3 ),
      RETRO_DESCRIPTOR_BLOCK( 4 ),

      { 0 },
   };

#undef RETRO_DESCRIPTOR_BLOCK

   environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, &inputDescriptors);
   
   unsigned dci_version = 0;
   if (environ_cb(RETRO_ENVIRONMENT_GET_DISK_CONTROL_INTERFACE_VERSION, &dci_version) && (dci_version >= 1))
      environ_cb(RETRO_ENVIRONMENT_SET_DISK_CONTROL_EXT_INTERFACE, &diskControlExt);
   else
      environ_cb(RETRO_ENVIRONMENT_SET_DISK_CONTROL_INTERFACE, &diskControl);

   // Keep as incomplete until rewind can be enabled at startup (snapshot size is 0 at that time)
   static uint32_t quirks = RETRO_SERIALIZATION_QUIRK_INCOMPLETE | RETRO_SERIALIZATION_QUIRK_MUST_INITIALIZE | RETRO_SERIALIZATION_QUIRK_CORE_VARIABLE_SIZE;
   environ_cb(RETRO_ENVIRONMENT_SET_SERIALIZATION_QUIRKS, &quirks);

   bool achievements = true;
   environ_cb(RETRO_ENVIRONMENT_SET_SUPPORT_ACHIEVEMENTS, &achievements);
}

void retro_deinit(void)
{
   // Clean the disk control context
   if (dc)
      dc_free(dc);

   // Clean legacy strings
   if (core_options_legacy_strings)
      free(core_options_legacy_strings);

   // Clean ZIP temp
   if (retro_temp_directory != NULL && path_is_directory(retro_temp_directory))
      remove_recurse(retro_temp_directory);
}

unsigned retro_api_version(void)
{
   return RETRO_API_VERSION;
}

void retro_set_controller_port_device( unsigned port, unsigned device )
{
   if (port < 5)
      vice_devices[port] = device;
}

void retro_get_system_info(struct retro_system_info *info)
{
#ifndef GIT_VERSION
#define GIT_VERSION ""
#endif
   memset(info, 0, sizeof(*info));
   info->library_name     = "VICE " CORE_NAME;
   info->library_version  = "3.3" GIT_VERSION;
#if defined(__XVIC__)
   info->valid_extensions = "20|40|60|a0|b0|d64|d71|d80|d81|d82|g64|g41|x64|t64|tap|prg|p00|crt|bin|zip|gz|d6z|d7z|d8z|g6z|g4z|x6z|cmd|m3u|vfl|vsf|nib|nbz";
#else
   info->valid_extensions = "d64|d71|d80|d81|d82|g64|g41|x64|t64|tap|prg|p00|crt|bin|zip|gz|d6z|d7z|d8z|g6z|g4z|x6z|cmd|m3u|vfl|vsf|nib|nbz";
#endif
   info->need_fullpath    = true;
   info->block_extract    = true;
}

double retro_get_aspect_ratio(unsigned int width, unsigned int height, bool pixel_aspect)
{
   static double ar;
   static double par;
   static int region = 0;
   region = retro_region;
   switch (opt_aspect_ratio)
   {
      case 1:
         region = RETRO_REGION_PAL;
         break;
      case 2:
         region = RETRO_REGION_NTSC;
         break;
      case 3:
         region = -1;
         par = 1;
         break;
   }

#if defined(__X64__) || defined(__X64SC__) || defined(__X128__) || defined(__XSCPU64__) || defined(__XCBM5x0__)
      if (region == RETRO_REGION_NTSC)
         par = (double)0.75000000;
      else
         par = (double)0.93650794;
      ar = ((double)width / (double)height) * par;
#if defined(__X128__)
      if (core_opt.C128ColumnKey == 0)
         ar = ((double)width / (double)height) / (double)2.0;
#endif
#elif defined(__XVIC__)
      if (region == RETRO_REGION_NTSC)
         par = ((double)1.50411479 / (double)2.0);
      else
         par = ((double)1.66574035 / (double)2.0);
      ar = ((double)width / (double)height) * par;
#elif defined(__XPLUS4__)
      if (region == RETRO_REGION_NTSC)
         par = (double)0.85760931;
      else
         par = (double)1.03743478;
      ar = ((double)width / (double)height) * par;
#else
      ar = (double)4 / (double)3;
#endif

   if (pixel_aspect)
      return par;
   if (opt_aspect_ratio == 3) // 1:1
      return ((double)width / (double)height);
   return ar;
}

void update_geometry(int mode)
{
   struct retro_system_av_info system_av_info;

#ifdef RETRO_DEBUG
   log_cb(RETRO_LOG_INFO, "Update Geometry: Old(%d,%d) New(%d,%d)\n", lastW, lastH, retroW, retroH);
#endif
   lastW = retroW;
   lastH = retroH;

   switch (mode)
   {
      case 0:
         /* Zoom mode init */
         zoom_mode_id_prev  = 0;
         zoomed_width       = retroW;
         zoomed_height      = retroH;
         zoomed_XS_offset   = 0;
         zoomed_YS_offset   = 0;
         retroXS_offset     = 0;
         retroYS_offset     = 0;

         system_av_info.geometry.base_width = retroW;
         system_av_info.geometry.base_height = retroH;
         system_av_info.geometry.aspect_ratio = retro_get_aspect_ratio(retroW, retroH, false);

         /* Update av_info only when PAL/NTSC change occurs */
         if (retro_region != retro_get_region())
         {
            retro_region = retro_get_region();
            retro_get_system_av_info(&system_av_info);
            environ_cb(RETRO_ENVIRONMENT_SET_SYSTEM_AV_INFO, &system_av_info);
            return;
         }
         break;

      case 1:
#if defined(__X64__) || defined(__X64SC__) || defined(__X128__) || defined(__XSCPU64__) || defined(__XCBM5x0__) || defined(__XVIC__) || defined(__XPLUS4__)
         if (zoom_mode_id != zoom_mode_id_prev)
         {
            zoom_mode_id_prev = zoom_mode_id;
#if defined(__X64__) || defined(__X64SC__) || defined(__X128__) || defined(__XSCPU64__) || defined(__XCBM5x0__)
            // PAL: 384x272, NTSC: 384x247, VIC-II: 320x200
            int zoom_width_max      = 320;
            int zoom_height_max     = 200;
#elif defined(__XVIC__)
            // PAL: 448x284, NTSC: 400x234, VIC: 352x184
            int zoom_width_max      = 352;
            int zoom_height_max     = 184;
#elif defined(__XPLUS4__)
            // PAL: 384x288, NTSC: 384x242, TED: 320x200
            int zoom_width_max      = 320;
            int zoom_height_max     = 200;
#endif
            int zoom_crop_width     = 0;
            int zoom_crop_height    = 0;
            int zoom_border_width   = 0;
            int zoom_border_height  = 0;

            double zoom_dar = 0;
            double zoom_par = retro_get_aspect_ratio(0, 0, true);

            switch (zoom_mode_id)
            {
               case 1:
               case 2:
               case 3:
                  switch (zoom_mode_id)
                  {
                     case 1: // Small
                        zoom_border_width     = 44;
                        zoom_border_height    = 36;
                        break;
                     case 2: // Medium
                        zoom_border_width     = 22;
                        zoom_border_height    = 18;
                        break;
                     case 3: // Maximum
                        break;
                  }

                  zoom_crop_width = retroW - zoom_width_max - zoom_border_width;
                  zoom_crop_height = retroH - zoom_height_max - zoom_border_height;

                  switch (zoom_mode_crop_id)
                  {
                     case 0: // Both
                        break;
                     case 1: // Vertical disables horizontal crop
                        zoom_crop_width = 0;
                        break;
                     case 2: // Horizontal disables vertical crop
                        zoom_crop_height = 0;
                        break;
                     case 3: // 16:9
                        zoom_dar = (double)16/9;
                        zoom_crop_width = retroW - ((double)(retroH - zoom_crop_height) * (double)zoom_dar / (double)zoom_par);
                        break;
                     case 4: // 16:10
                        zoom_dar = (double)16/10;
                        zoom_crop_width = retroW - ((double)(retroH - zoom_crop_height) * (double)zoom_dar / (double)zoom_par);
                        break;
                     case 5: // 4:3
                        zoom_dar = (double)4/3;
                        zoom_crop_height = retroH - zoom_height_max - ((double)zoom_border_height * (double)zoom_dar / (double)zoom_par);
                        zoom_crop_width = retroW - ((double)(retroH - zoom_crop_height) * (double)zoom_dar / (double)zoom_par);
                        if (retroW - zoom_crop_width <= zoom_width_max)
                           zoom_crop_height = retroH - ((double)(zoom_width_max) / (double)zoom_dar * (double)zoom_par);
                        break;
                     case 6: // 5:4
                        zoom_dar = (double)5/4;
                        zoom_crop_height = retroH - zoom_height_max - ((double)zoom_border_height * (double)zoom_dar / (double)zoom_par);
                        zoom_crop_width = retroW - ((double)(retroH - zoom_crop_height) * (double)zoom_dar / (double)zoom_par);
                        if (retroW - zoom_crop_width <= zoom_width_max)
                           zoom_crop_height = retroH - ((double)(zoom_width_max) / (double)zoom_dar * (double)zoom_par);
                        break;
                  }

                  if (retroW - zoom_crop_width < zoom_width_max)
                     zoom_crop_width = retroW - zoom_width_max;
                  if (retroH - zoom_crop_height < zoom_height_max)
                     zoom_crop_height = retroH - zoom_height_max;

                  if (zoom_crop_width < 0)
                     zoom_crop_width = 0;
                  if (zoom_crop_height < 0)
                     zoom_crop_height = 0;

                  zoomed_width        = retroW - zoom_crop_width;
                  zoomed_height       = retroH - zoom_crop_height;
                  //printf("zoom: dar:%f par:%f - x-%3d y-%3d = %3dx%3d = %f * %f = %f\n", zoom_dar, zoom_par, zoom_crop_width, zoom_crop_height, zoomed_width, zoomed_height, ((double)zoomed_width / (double)zoomed_height), zoom_par, ((double)zoomed_width / (double)zoomed_height * zoom_par));

#if defined(__X64__) || defined(__X64SC__) || defined(__X128__) || defined(__XSCPU64__) || defined(__XCBM5x0__)
                  zoomed_XS_offset    = (zoom_crop_width > 1) ? (zoom_crop_width / 2) : 0;
                  zoomed_YS_offset    = (zoom_crop_height > 1) ? (zoom_crop_height / 2) - ((retro_region == RETRO_REGION_PAL) ? 1 : 0) : 0;
#elif defined(__XVIC__)
                  zoomed_XS_offset    = (zoom_crop_width > 1) ? (zoom_crop_width / 2) - ((retro_region == RETRO_REGION_PAL) ? 0 : -8) : 0;
                  zoomed_YS_offset    = (zoom_crop_height > 1) ? (zoom_crop_height / 2) - ((retro_region == RETRO_REGION_PAL) ? 2 : 3) : 0;
#elif defined(__XPLUS4__)
                  zoomed_XS_offset    = (zoom_crop_width > 1) ? (zoom_crop_width / 2) : 0;
                  zoomed_YS_offset    = (zoom_crop_height > 1) ? (zoom_crop_height / 2) - ((retro_region == RETRO_REGION_PAL) ? 4 : 3) : 0;
#endif
                  break;

               case 4:
                  zoom_crop_width    = manual_crop_left + manual_crop_right;
                  zoom_crop_height   = manual_crop_top + manual_crop_bottom;

                  zoomed_width       = retroW - zoom_crop_width;
                  zoomed_height      = retroH - zoom_crop_height;
                  zoomed_XS_offset   = manual_crop_left;
                  zoomed_YS_offset   = manual_crop_top;
                  break;

               default:
                  zoomed_width        = retroW;
                  zoomed_height       = retroH;
                  zoomed_XS_offset    = 0;
                  zoomed_YS_offset    = 0;
                  retroXS_offset      = 0;
                  retroYS_offset      = 0;
                  break;
            }

            system_av_info.geometry.base_width = zoomed_width;
            system_av_info.geometry.base_height = zoomed_height;
            system_av_info.geometry.aspect_ratio = retro_get_aspect_ratio(zoomed_width, zoomed_height, false);
         }
#endif
         break;
   }
   environ_cb(RETRO_ENVIRONMENT_SET_GEOMETRY, &system_av_info);
}

void retro_get_system_av_info(struct retro_system_av_info *info)
{
   /* need to do this here because core option values are not available in retro_init */
   if (pix_bytes == 4)
   {
      enum retro_pixel_format fmt = RETRO_PIXEL_FORMAT_XRGB8888;
      if (!environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt))
      {
         log_cb(RETRO_LOG_INFO, "XRGB8888 is not supported. Trying RGB565.\n");
         fmt = RETRO_PIXEL_FORMAT_RGB565;
         pix_bytes = 2;
         if (!environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt))
         {
            log_cb(RETRO_LOG_INFO, "RGB565 is not supported.\n");
            exit(0);//return false;
         }
      }
   }

   info->geometry.max_width = defaultW;
   info->geometry.max_height = defaultH;
   info->geometry.base_width = retroW;
   info->geometry.base_height = retroH;
   info->geometry.aspect_ratio = retro_get_aspect_ratio(retroW, retroH, false);
   info->timing.sample_rate = core_opt.SoundSampleRate;
   prev_sound_sample_rate = core_opt.SoundSampleRate;

   // Remember region for av_info update
   retro_region = retro_get_region();

#if defined(__X64__) || defined(__X64SC__)
   retro_refresh = (retro_region == RETRO_REGION_PAL) ? C64_PAL_RFSH_PER_SEC : C64_NTSC_RFSH_PER_SEC;
#elif defined(__X128__)
   retro_refresh = (retro_region == RETRO_REGION_PAL) ? C128_PAL_RFSH_PER_SEC : C128_NTSC_RFSH_PER_SEC;
#elif defined(__XCBM2__)
   retro_refresh = (retro_region == RETRO_REGION_PAL) ? C610_PAL_RFSH_PER_SEC : C610_NTSC_RFSH_PER_SEC;
#elif defined(__XCBM5x0__)
   retro_refresh = (retro_region == RETRO_REGION_PAL) ? C500_PAL_RFSH_PER_SEC : C500_NTSC_RFSH_PER_SEC;
#elif defined(__XPET__)
   retro_refresh = (retro_region == RETRO_REGION_PAL) ? PET_PAL_RFSH_PER_SEC : PET_NTSC_RFSH_PER_SEC;
#elif defined(__XPLUS4__)
   retro_refresh = (retro_region == RETRO_REGION_PAL) ? PLUS4_PAL_RFSH_PER_SEC : PLUS4_NTSC_RFSH_PER_SEC;
#elif defined(__XSCPU64__)
   retro_refresh = (retro_region == RETRO_REGION_PAL) ? SCPU64_PAL_RFSH_PER_SEC : SCPU64_NTSC_RFSH_PER_SEC;
#elif defined(__XVIC__)
   retro_refresh = (retro_region == RETRO_REGION_PAL) ? VIC20_PAL_RFSH_PER_SEC : VIC20_NTSC_RFSH_PER_SEC;
#endif
   info->timing.fps = retro_refresh;
}

void retro_set_video_refresh(retro_video_refresh_t cb)
{
   video_cb = cb;
}

void retro_set_audio_sample(retro_audio_sample_t cb)
{
   audio_cb = cb;
}

void retro_audio_cb(short l, short r)
{
   audio_cb(l, r);
}

void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb)
{
   audio_batch_cb = cb;
}

void retro_audio_batch_cb(const int16_t *data, size_t frames)
{
   audio_batch_cb(data, frames);
}

void retro_audio_render(signed short int *sound_buffer, int sndbufsize)
{
   int x;
#if 1
   for (x=0; x<sndbufsize; x++) audio_cb(sound_buffer[x], sound_buffer[x]); // Mono output
#else
   audio_batch_cb(sound_buffer, sndbufsize/2); // Stereo output, fails with reSIDfp!
#endif
}



void retro_run(void)
{
   bool updated = false;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updated) && updated)
      update_variables();

   if (retro_ui_finalized)
   {
#if defined(__X64__) || defined(__X64SC__) || defined(__XSCPU64__)
      /* Set model when requested */
      if (opt_model_auto == 1 && request_model_set > -1)
      {
         if (request_model_prev != request_model_set)
         {
             if (request_model_set == C64MODEL_C64_NTSC || request_model_set == C64MODEL_C64C_NTSC)
                fprintf(stdout, "[libretro-vice]: Forcing NTSC mode\n");
             else if (request_model_set == C64MODEL_C64_PAL || request_model_set == C64MODEL_C64C_PAL)
                fprintf(stdout, "[libretro-vice]: Forcing PAL mode\n");

             c64model_set(request_model_set);
             request_model_prev = request_model_set;
         }
         opt_model_auto = 2;
      }
#endif
      /* Update work disk */
      if (request_update_work_disk)
         update_work_disk();

      /* Update samplerate if changed by core option */
      if (prev_sound_sample_rate != core_opt.SoundSampleRate)
      {
         prev_sound_sample_rate = core_opt.SoundSampleRate;

         /* Ensure audio rendering is reinitialized on next use. */
         sound_close();

         /* Reset zoom for proper aspect ratio after av_info change */
         zoom_mode_id_prev = -1;

         struct retro_system_av_info system_av_info;
         retro_get_system_av_info(&system_av_info);
         environ_cb(RETRO_ENVIRONMENT_SET_SYSTEM_AV_INFO, &system_av_info);
      }

      /* Update geometry if model or zoom mode changes */
      if ((lastW == retroW && lastH == retroH) && zoom_mode_id != zoom_mode_id_prev)
         update_geometry(1);
      else if (lastW != retroW || lastH != retroH)
         update_geometry(0);
   }

   if (retro_ui_finalized && !prev_ui_finalized)
   {
      log_cb(RETRO_LOG_INFO, "UI finalized now\n");
      prev_ui_finalized = 1;
   }

   if (show_queued_msg != 0)
   {
      struct retro_message rmsg;
      rmsg.msg = queued_msg;
      rmsg.frames = show_queued_msg;
      environ_cb(RETRO_ENVIRONMENT_SET_MESSAGE, &rmsg);
      show_queued_msg = 0;
   }

   if (runstate == RUNSTATE_FIRST_START)
   {
      /* this is only done once after just loading the core from scratch and starting it */
#ifdef RETRO_DEBUG
      log_cb(RETRO_LOG_INFO, "First time we return from retro_run()!\n");
#endif
      runstate = RUNSTATE_RUNNING;
      pre_main();
      reload_restart();
      return;
   } 
   else if (runstate == RUNSTATE_LOADED_CONTENT)
   {
      /* Load content was called while core was already running, just do a reset with autostart */
      reload_restart();
      /* After retro_load_game, get_system_av_info is always called by the frontend */
      /* resetting the aspect to 4/3 etc. So we inform the frontend of the actual */
      /* current aspect ratio and screen size again here */
      update_geometry(0);
      runstate = RUNSTATE_RUNNING;
   } 

   /* Input poll */
   retro_poll_event();

   /* Measure frame-time and time between frames to render as much frames as possible when warp is enabled. Does not work
      perfectly as the time needed by the framework cannot be accounted, but should not reduce amount of actually rendered
      frames too much. */
   {
      static struct retro_perf_callback pcb;
      if (!pcb.get_time_usec)
         environ_cb(RETRO_ENVIRONMENT_GET_PERF_INTERFACE, &pcb);

      static unsigned int f_time = 0, f_minimum = 1;
      if (!f_time)
      {
          f_time = 1000000 / retro_refresh;
          f_minimum = f_time / 100;
      }

      if (pcb.get_time_usec)
         retro_now = pcb.get_time_usec();
      else
         retro_now += f_time;

      for (int frame_count = 1; frame_count <= (retro_warp_mode_enabled() ? (f_time / f_minimum) : 1); ++frame_count)
      {
         while (cpuloop)
            maincpu_mainloop_retro();
         cpuloop = 1;
      }
   }

   /* Show VKBD */
   if (SHOWKEY==1)
      print_virtual_kbd(retro_bmp);

   /* Finalize zoom offsets */
   if (zoomed_XS_offset != retroXS_offset || zoomed_YS_offset != retroYS_offset)
   {
      retroXS_offset = zoomed_XS_offset;
      retroYS_offset = zoomed_YS_offset;
   }

   /* Set volume back to maximum after starting with mute, due to ReSID 6581 init pop */
   if (sound_volume_counter > 0)
   {
      sound_volume_counter--;
      if (sound_volume_counter == 0)
         log_resources_set_int("SoundVolume", 100);
   }

   /* Statusbar disk display timer */
   if (imagename_timer > 0)
      imagename_timer--;

   video_cb(retro_bmp+(retroXS_offset*pix_bytes/2)+(retroYS_offset*(retroW<<(pix_bytes/4))), zoomed_width, zoomed_height, retroW<<(pix_bytes/2));
}

bool retro_load_game(const struct retro_game_info *info)
{
   char *local_path;
   if (info)
   {
      /* Special unicode chars won't work internally in VICE without conversion */
      local_path = utf8_to_local_string_alloc(info->path);
      if (local_path)
      {
         process_cmdline(local_path);
         free(local_path);
         local_path = NULL;
      }
      else
         return false;
   }

   update_variables();

#if defined(__XVIC__) || defined(__XPET__) || defined(__XCBM2__)
   /* Joyport limit has to apply always */
   cur_port = 1;
   cur_port_locked = 1;
#endif

   if (runstate == RUNSTATE_RUNNING)
   {
      /* load game was called while core is already running */
      /* so we update runstate and do the deferred autostart_reset in retro_run */
      /* the autostart_reset has to be deferred because a bunch of other init stuff */
      /* is done between retro_load_game() and retro_run() at a higher level */
      runstate = RUNSTATE_LOADED_CONTENT;
   }

   return true;
}

void retro_unload_game(void)
{
   file_system_detach_disk(8);
   tape_image_detach(1);
   cartridge_detach_image(-1);
   dc_reset(dc);
   free(autostartString);
   autostartString = NULL;
}

unsigned retro_get_region(void)
{
    int machine_sync = 0;
    if (retro_ui_finalized)
        resources_get_int("MachineVideoStandard", &machine_sync);
    switch (machine_sync)
    {
        default:
        case MACHINE_SYNC_PAL:
        case MACHINE_SYNC_PALN:
            return RETRO_REGION_PAL;
            break;
        case MACHINE_SYNC_NTSC:
        case MACHINE_SYNC_NTSCOLD:
            return RETRO_REGION_NTSC;
            break;
    }
}

bool retro_load_game_special(unsigned type, const struct retro_game_info *info, size_t num)
{
   (void)type;
   (void)info;
   (void)num;
   return false;
}

/* CPU traps ensure we are never saving snapshots or loading them in the middle of a cpu instruction.
   Without this, savestate corruption occurs.
*/

static void save_trap(uint16_t addr, void *success)
{
   int save_disks;
   int drive_type;
   resources_get_int("Drive8Type", &drive_type);
   save_disks = (drive_type < 1550) ? 1 : 0;

   /* params: stream, save_roms, save_disks, event_mode */
   if (machine_write_snapshot_to_stream(snapshot_stream, 0, save_disks, 0) >= 0)
      *((int *)success) = 1;
   else
      *((int *)success) = 0;
   save_trap_happened = 1;
}

static void load_trap(uint16_t addr, void *success)
{
   /* params: stream, event_mode */
   if (machine_read_snapshot_from_stream(snapshot_stream, 0) >= 0)
      *((int *)success) = 1;
   else
      *((int *)success) = 0;
   load_trap_happened = 1;
}

size_t retro_serialize_size(void)
{
   long snapshot_size = 0;
   if (retro_ui_finalized)
   {
      snapshot_stream = snapshot_memory_write_fopen(NULL, 0);
      int success = 0;
      interrupt_maincpu_trigger_trap(save_trap, (void *)&success);
      save_trap_happened = 0;
      while (!save_trap_happened)
         maincpu_mainloop_retro();
      if (snapshot_stream != NULL)
      {
         if (success)
         {
            snapshot_fseek(snapshot_stream, 0, SEEK_END);
            snapshot_size = snapshot_ftell(snapshot_stream);
         }
         else
         {
            log_cb(RETRO_LOG_INFO, "Failed to calculate snapshot size\n");
         }
         snapshot_fclose(snapshot_stream);
         snapshot_stream = NULL;
      }
   }
   return snapshot_size;
}

bool retro_serialize(void *data_, size_t size)
{
   if (retro_ui_finalized)
   {
      snapshot_stream = snapshot_memory_write_fopen(data_, size);
      int success = 0;
      interrupt_maincpu_trigger_trap(save_trap, (void *)&success);
      save_trap_happened = 0;
      while (!save_trap_happened)
         maincpu_mainloop_retro();
      if (snapshot_stream != NULL)
      {
         snapshot_fclose(snapshot_stream);
         snapshot_stream = NULL;
      }
      if (success)
      {
         return true;
      }
      log_cb(RETRO_LOG_INFO, "Failed to serialize snapshot\n");
   }
   return false;
}

bool retro_unserialize(const void *data_, size_t size)
{
   if (retro_ui_finalized)
   {
      resources_set_int("WarpMode", 0);
      snapshot_stream = snapshot_memory_read_fopen(data_, size);
      int success = 0;
      interrupt_maincpu_trigger_trap(load_trap, (void *)&success);
      load_trap_happened = 0;
      while (!load_trap_happened)
         maincpu_mainloop_retro();
      if (snapshot_stream != NULL)
      {
         snapshot_fclose(snapshot_stream);
         snapshot_stream = NULL;
      }
      if (success)
      {
         return true;
      }
      log_cb(RETRO_LOG_INFO, "Failed to unserialize snapshot\n");
   }
   return false;
}

void *retro_get_memory_data(unsigned id)
{
   if (id == RETRO_MEMORY_SYSTEM_RAM)
      return mem_ram;
   return NULL;
}

size_t retro_get_memory_size(unsigned id)
{
   if (id == RETRO_MEMORY_SYSTEM_RAM)
      return g_mem_ram_size;
   return 0;
}

void retro_cheat_reset(void) {}

void retro_cheat_set(unsigned index, bool enabled, const char *code)
{
   (void)index;
   (void)enabled;
   (void)code;
}

