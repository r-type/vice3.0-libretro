#ifndef LIBRETRO_CORE_H
#define LIBRETRO_CORE_H 1

#include <stdint.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <dirent.h>

#include "vkbd.h"
#include "retroglue.h"
#include "retro_files.h"
#include "retro_strings.h"
#include "retro_disk_control.h"
#include "string/stdstring.h"
#include "file/file_path.h"
#include "compat/strcasestr.h"

#define MATRIX(a,b) (((a) << 3) | (b))
#define RGB565(r, g, b) ((((r>>3)<<11) | ((g>>2)<<5) | (b>>3)))
#define RGB888(r, g, b) (((r * 255 / 31) << 16) | ((g * 255 / 31) << 8) | (b * 255 / 31))
#define ARGB888(a, r, g, b) ((a << 24) | (r << 16) | (g << 8) | b)

//DEVICES
#define RETRO_DEVICE_VICE_KEYBOARD RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_KEYBOARD, 0)
#define RETRO_DEVICE_VICE_JOYSTICK RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_JOYPAD, 1)

//LOG
#if  defined(__ANDROID__) || defined(ANDROID)
#include <android/log.h>
#define LOG_TAG "RetroArch.vice"
#define LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#else
#define LOGI printf
#endif

//TYPES
#define UINT16 uint16_t
#define UINT32 uint32_t
typedef uint32_t uint32;
typedef uint8_t uint8;

//SCREEN
#if defined(__X128__)
#define WINDOW_WIDTH    856
#define WINDOW_HEIGHT   312
#elif defined(__PET__) || defined(__CBM2__)
#define WINDOW_WIDTH    704
#define WINDOW_HEIGHT   288
#elif defined(__VIC20__)
#define WINDOW_WIDTH    448
#define WINDOW_HEIGHT   288
#else
#define WINDOW_WIDTH    384
#define WINDOW_HEIGHT   288
#endif
#define RETRO_BMP_SIZE  (WINDOW_WIDTH * WINDOW_HEIGHT * 2)
extern unsigned short int retro_bmp[RETRO_BMP_SIZE];
extern int pix_bytes;

//VKBD
#define NPLGN 11
#define NLIGN 7
#define NLETT 5

//#define POINTER_DEBUG
#ifdef POINTER_DEBUG
extern int pointer_x;
extern int pointer_y;
#endif

extern int vkey_pos_x;
extern int vkey_pos_y;
extern int vkey_pressed;
extern int vkey_sticky;
extern int vkey_sticky1;
extern int vkey_sticky2;

extern int vkbd_x_min;
extern int vkbd_x_max;
extern int vkbd_y_min;
extern int vkbd_y_max;

//STATUSBAR
#define STATUSBAR_BOTTOM    0x01
#define STATUSBAR_TOP       0x02
#define STATUSBAR_BASIC     0x04
#define STATUSBAR_MINIMAL   0x08

//VARIABLES
extern int cpuloop;
extern int retroXS;
extern int retroYS;
extern int retroXS_offset;
extern int retroYS_offset;
extern int retroH;
extern int retroW;
extern unsigned int zoomed_width;
extern unsigned int zoomed_height;
extern unsigned int zoomed_XS_offset;
extern unsigned int zoomed_YS_offset;
extern int imagename_timer;
extern unsigned int opt_statusbar;
extern unsigned int cur_port;
extern int RETROUSERPORTJOY;

//FUNCS
extern void maincpu_mainloop_retro(void);
extern long GetTicks(void);

enum {
	RUNSTATE_FIRST_START = 0,
	RUNSTATE_LOADED_CONTENT,
	RUNSTATE_RUNNING,
};
#endif
