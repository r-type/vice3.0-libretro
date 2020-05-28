/*
 * ui.c - PSP user interface.
 *
 * Written by
 *  Akop Karapetyan <dev@psp.akop.org>
 *
 * This file is part of VICE, the Versatile Commodore Emulator.
 * See README for copyright notice.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307  USA.
 *
 */

#include "vice.h"
#include "machine.h"
#include "cmdline.h"
#include "uistatusbar.h"
#include "resources.h"
#include "sid.h"
#include "sid-resources.h"
#include "util.h"
#if !defined(__XCBM5x0__)
#include "userport_joystick.h"
#endif
#if defined(__XVIC__)
#include "c64model.h"
#include "vic20model.h"
#include "vic20mem.h"
#elif defined(__XPLUS4__)
#include "c64model.h"
#include "plus4model.h"
#elif defined(__X128__)
#include "c64model.h"
#include "c128model.h"
#include "c128rom.h"
#include "c128mem.h"
#include "c128kernal.h"
#include "c128kernal64.h"
BYTE c128memrom_kernal128_rom_original[C128_KERNAL_ROM_IMAGE_SIZE] = {0};
BYTE c128memrom_kernal64_rom_original[C128_KERNAL64_ROM_SIZE] = {0};
#elif defined(__XPET__)
#include "petmodel.h"
#include "keyboard.h"
#elif defined(__XCBM2__) || defined(__XCBM5x0__)
#include "cbm2model.h"
#else
#include "c64model.h"
#include "c64rom.h"
#include "c64mem.h"
#include "c64memrom.h"
BYTE c64memrom_kernal64_rom_original[C64_KERNAL_ROM_SIZE] = {0};
#endif
#include "archdep.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "retro_files.h"

int RETROTDE=0;
int RETRODSE=0;
int RETROSIDENGINE=0;
int RETROSIDMODL=0;
int RETROSIDEXTRA=0;
int RETRORESIDSAMPLING=0;
int RETROSOUNDSAMPLERATE=0;
int RETRORESIDPASSBAND=0;
int RETRORESIDGAIN=0;
int RETRORESIDFILTERBIAS=0;
int RETRORESID8580FILTERBIAS=0;
int RETROAUDIOLEAK=0;
int RETROMODEL=0;
int RETROAUTOSTARTWARP=0;
int RETROUSERPORTJOY=-1;
char RETROEXTPALNAME[RETRO_PATH_MAX]="default";
#if defined(__X128__)
int RETROC128COLUMNKEY=1;
int RETROC128GO64=0;
#endif
#if defined(__XVIC__)
int RETROVIC20MEM=0;
int vic20mem_forced=-1;
#endif
#if defined(__X64__) || defined(__X64SC__) || defined(__X128__) || defined(__XSCPU64__) || defined(__XCBM5x0__)
int RETROVICIICOLORGAMMA=2200;
int RETROVICIICOLORSATURATION=1250;
int RETROVICIICOLORCONTRAST=1250;
int RETROVICIICOLORBRIGHTNESS=1000;
#endif

int retro_ui_finalized = 0;
int cur_port_locked = 0; /* 0: not forced by filename 1: forced by filename */
extern unsigned int opt_jiffydos;
extern unsigned int opt_autoloadwarp;
extern char retro_system_data_directory[RETRO_PATH_MAX];

static const cmdline_option_t cmdline_options[] = {
     { NULL }
};

/* Initialization  */
int ui_resources_init(void)
{
   if (machine_class != VICE_MACHINE_VSID)
      return uistatusbar_init_resources();
   return 0;
}

void ui_resources_shutdown(void)
{
}

int ui_init(int argc, char **argv)
{
    return 0;
}

void ui_shutdown(void)
{
}

void ui_check_mouse_cursor(void)
{
}

void ui_error(const char *format, ...)
{
   char text[512];	   	
   va_list	ap;	

   if (format == NULL)return;		

   va_start(ap,format );		
   vsprintf(text, format, ap);	
   va_end(ap);	
   fprintf(stderr, "ui_error: %s\n", text);
}

int ui_emulation_is_paused(void)
{
    return 0;
}

void ui_pause_emulation(int flag)
{
}

/* Update all the menus according to the current settings.  */

void ui_update_menus(void)
{
}

int ui_extend_image_dialog(void)
{
   return 0;
}

void ui_dispatch_events(void)
{
}

int ui_cmdline_options_init(void)
{
   return cmdline_register_options(cmdline_options);
}

int ui_init_finish(void)
{
   return 0;
}

extern int log_resources_set_int(const char *name, int value);
extern int log_resources_set_string(const char *name, const char* value);

int ui_init_finalize(void)
{
   /* Dump machine specific defaults for 'vicerc' usage, if not already dumped */
   char resources_dump_path[RETRO_PATH_MAX] = {0};
   snprintf(resources_dump_path, sizeof(resources_dump_path), "%s%s%s%s", retro_system_data_directory, FSDEV_DIR_SEP_STR, "vicerc-dump-", machine_get_name());
   if (!util_file_exists(resources_dump_path))
      resources_dump(resources_dump_path);

   /* Sensible defaults */
   log_resources_set_int("Mouse", 1);
   log_resources_set_int("AutostartPrgMode", 1);
   log_resources_set_int("VirtualDevices", 1);
   log_resources_set_int("Printer4", 1);

   /* Mute sound at startup to hide 6581 ReSID init pop, and set back to 100 in retro_run() after 3 frames */
   log_resources_set_int("SoundVolume", 0);

#if defined(__XCBM2__) || defined(__XPET__)
   log_resources_set_int("CrtcFilter", 0);
   log_resources_set_int("CrtcStretchVertical", 0);
#endif

   /* Core options */
#if defined(__XVIC__)
   if (strcmp(RETROEXTPALNAME, "default") == 0)
      log_resources_set_int("VICExternalPalette", 0);
   else
   {
      log_resources_set_int("VICExternalPalette", 1);
      log_resources_set_string("VICPaletteFile", RETROEXTPALNAME);
   }
#elif defined(__XPLUS4__)
   if (strcmp(RETROEXTPALNAME, "default") == 0)
      log_resources_set_int("TEDExternalPalette", 0);
   else
   {
      log_resources_set_int("TEDExternalPalette", 1);
      log_resources_set_string("TEDPaletteFile", RETROEXTPALNAME);
   }
#elif defined(__XPET__)
   if (strcmp(RETROEXTPALNAME, "default") == 0)
      log_resources_set_int("CrtcExternalPalette", 0);
   else
   {
      log_resources_set_int("CrtcExternalPalette", 1);
      log_resources_set_string("CrtcPaletteFile", RETROEXTPALNAME);
   }
#elif defined(__XCBM2__)
   if (strcmp(RETROEXTPALNAME, "default") == 0)
      log_resources_set_int("CrtcExternalPalette", 0);
   else
   {
      log_resources_set_int("CrtcExternalPalette", 1);
      log_resources_set_string("CrtcPaletteFile", RETROEXTPALNAME);
   }
#else
   if (strcmp(RETROEXTPALNAME, "default") == 0)
      log_resources_set_int("VICIIExternalPalette", 0);
   else
   {
      log_resources_set_int("VICIIExternalPalette", 1);
      log_resources_set_string("VICIIPaletteFile", RETROEXTPALNAME);
   }
#endif

#if defined(__X64__) || defined(__X64SC__) || defined(__X128__) || defined(__XSCPU64__) || defined(__XCBM5x0__)
   log_resources_set_int("VICIIColorGamma", RETROVICIICOLORGAMMA);
   log_resources_set_int("VICIIColorSaturation", RETROVICIICOLORSATURATION);
   log_resources_set_int("VICIIColorContrast", RETROVICIICOLORCONTRAST);
   log_resources_set_int("VICIIColorBrightness", RETROVICIICOLORBRIGHTNESS);
#endif

#if !defined(__XCBM5x0__)
   if (RETROUSERPORTJOY==-1)
      log_resources_set_int("UserportJoy", 0);
   else
   {
      log_resources_set_int("UserportJoy", 1);
      log_resources_set_int("UserportJoyType", RETROUSERPORTJOY);
   }
#endif

   if (RETROTDE)
      log_resources_set_int("DriveTrueEmulation", 1);
   else
      log_resources_set_int("DriveTrueEmulation", 0);

   if (RETRODSE)
   {
      log_resources_set_int("DriveSoundEmulation", 1);
      log_resources_set_int("DriveSoundEmulationVolume", RETRODSE);
   }
   else
      log_resources_set_int("DriveSoundEmulation", 0);
   if (opt_autoloadwarp)
      log_resources_set_int("DriveSoundEmulationVolume", 0);

   log_resources_set_int("AutostartWarp", RETROAUTOSTARTWARP);

#if defined(__X64__) || defined(__X64SC__) || defined(__X128__) || defined(__XSCPU64__)
   log_resources_set_int("VICIIAudioLeak", RETROAUDIOLEAK);
#elif defined(__XVIC__)
   log_resources_set_int("VICAudioLeak", RETROAUDIOLEAK);
#endif

#if defined(__X64__) || defined(__X64SC__) || defined(__X128__) || defined(__XSCPU64__)
   // Replace kernal always from backup, because kernal loading replaces the embedded variable
#if defined(__X64__) || defined(__X64SC__)
   memcpy(c64memrom_kernal64_rom, c64memrom_kernal64_rom_original, C64_KERNAL_ROM_SIZE);
#elif defined(__X128__)
   memcpy(c128kernal64_embedded, c128memrom_kernal64_rom_original, C128_KERNAL64_ROM_SIZE);
   memcpy(kernal_int, c128memrom_kernal128_rom_original, C128_KERNAL_ROM_IMAGE_SIZE);
#endif
   char tmp_str[RETRO_PATH_MAX] = {0};
   if (opt_jiffydos)
   {
#if defined(__X64__) || defined(__X64SC__)
      snprintf(tmp_str, sizeof(tmp_str), "%s%c%s", retro_system_data_directory, FSDEV_DIR_SEP_CHR, "JiffyDOS_C64.bin");
      log_resources_set_string("KernalName", (const char*)tmp_str);
#elif defined(__X128__)
      snprintf(tmp_str, sizeof(tmp_str), "%s%c%s", retro_system_data_directory, FSDEV_DIR_SEP_CHR, "JiffyDOS_C64.bin");
      log_resources_set_string("Kernal64Name", (const char*)tmp_str);
      snprintf(tmp_str, sizeof(tmp_str), "%s%c%s", retro_system_data_directory, FSDEV_DIR_SEP_CHR, "JiffyDOS_C128.bin");
      log_resources_set_string("KernalIntName", (const char*)tmp_str);
#endif
      snprintf(tmp_str, sizeof(tmp_str), "%s%c%s", retro_system_data_directory, FSDEV_DIR_SEP_CHR, "JiffyDOS_1541-II.bin");
      log_resources_set_string("DosName1541", (const char*)tmp_str);
      snprintf(tmp_str, sizeof(tmp_str), "%s%c%s", retro_system_data_directory, FSDEV_DIR_SEP_CHR, "JiffyDOS_1571_repl310654.bin");
      log_resources_set_string("DosName1571", (const char*)tmp_str);
      snprintf(tmp_str, sizeof(tmp_str), "%s%c%s", retro_system_data_directory, FSDEV_DIR_SEP_CHR, "JiffyDOS_1581.bin");
      log_resources_set_string("DosName1581", (const char*)tmp_str);
   }
   else
   {
#if defined(__X64__) || defined(__X64SC__)
      log_resources_set_string("KernalName", "kernal");
#elif defined(__X128__)
      log_resources_set_string("Kernal64Name", "kernal64");
      log_resources_set_string("KernalIntName", "kernal");
#endif
      log_resources_set_string("DosName1541", "dos1541");
      log_resources_set_string("DosName1571", "dos1571");
      log_resources_set_string("DosName1581", "dos1581");
   }
#endif

#if defined(__XPET__)
   petmodel_set(RETROMODEL);
   keyboard_init();
#elif defined(__XCBM2__) || defined(__XCBM5x0__)
   cbm2model_set(RETROMODEL);
#elif defined(__XVIC__)
   vic20model_set(RETROMODEL);
#elif defined(__XPLUS4__)
   plus4model_set(RETROMODEL);
#elif defined(__X128__)
   c128model_set(RETROMODEL);
#else
   c64model_set(RETROMODEL);
#endif

#if !defined(__XPET__) && !defined(__XPLUS4__) && !defined(__XVIC__)
   if (RETROSIDMODL == 0xff)
      resources_set_int("SidEngine", RETROSIDENGINE);
   else
      sid_set_engine_model(RETROSIDENGINE, RETROSIDMODL);
   log_resources_set_int("SidResidSampling", RETRORESIDSAMPLING);
   log_resources_set_int("SidResidPassband", RETRORESIDPASSBAND);
   log_resources_set_int("SidResidGain", RETRORESIDGAIN);
   log_resources_set_int("SidResidFilterBias", RETRORESIDFILTERBIAS);
   log_resources_set_int("SidResid8580Passband", RETRORESIDPASSBAND);
   log_resources_set_int("SidResid8580Gain", RETRORESIDGAIN);
   log_resources_set_int("SidResid8580FilterBias", RETRORESID8580FILTERBIAS);

   if (RETROSIDEXTRA)
   {
      log_resources_set_int("SidStereo", 1);
      log_resources_set_int("SidStereoAddressStart", RETROSIDEXTRA);
   }
   else
      log_resources_set_int("SidStereo", 0);
#endif

#if defined(__X128__)
   log_resources_set_int("Go64Mode", RETROC128GO64);
   log_resources_set_int("C128ColumnKey", RETROC128COLUMNKEY);
#endif

#if defined(__XVIC__)
   unsigned int vic20mem = 0;
   vic20mem = (vic20mem_forced > -1) ? vic20mem_forced : RETROVIC20MEM;

   // Super VIC uses memory blocks 1+2 by default
   if (!vic20mem && RETROMODEL == VIC20MODEL_VIC21)
      vic20mem = 3;

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
#endif
   retro_ui_finalized = 1;
   return 0;
}

char* ui_get_file(const char *format,...)
{
   return NULL;
}


#if defined(__X64__)
int c64ui_init_early(void)
{
   memcpy(c64memrom_kernal64_rom_original, c64memrom_kernal64_rom, C64_KERNAL_ROM_SIZE);
   return 0;
}
#elif defined(__X64SC__)
int c64scui_init_early(void)
{
   memcpy(c64memrom_kernal64_rom_original, c64memrom_kernal64_rom, C64_KERNAL_ROM_SIZE);
   return 0;
}
#elif defined(__XSCPU64__)
int scpu64ui_init_early(void)
{
   return 0;
}
#elif defined(__X128__)
int c128ui_init_early(void)
{
   memcpy(c128memrom_kernal128_rom_original, kernal_int, C128_KERNAL_ROM_IMAGE_SIZE);
   memcpy(c128memrom_kernal64_rom_original, c128kernal64_embedded, C128_KERNAL64_ROM_SIZE);
   return 0;
}
#elif defined(__XVIC__)
int vic20ui_init_early(void)
{
   return 0;
}
#elif defined(__XPET__)
int petui_init_early(void)
{
   return 0;
}
#elif defined(__XPLUS4__)
int plus4ui_init_early(void)
{
   return 0;
}
#elif defined(__XCBM2__)
int cbm2ui_init_early(void)
{
   return 0;
}
#elif defined(__XCBM5x0__)
int cbm5x0ui_init_early(void)
{
   return 0;
}
#endif
