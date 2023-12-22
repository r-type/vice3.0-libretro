/*
 * gfxoutput.c - Graphics output driver.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
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

#if 0
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#endif

#include "archdep.h"
#include "gfxoutput.h"
#if 0
#include "lib.h"
#include "log.h"
#endif
#if 0
#include "bmpdrv.h"
#include "gifdrv.h"
#include "iffdrv.h"
#include "nativedrv.h"
#include "pcxdrv.h"
#include "ppmdrv.h"
#include "godotdrv.h"

#ifdef HAVE_PNG
#include "pngdrv.h"
#endif

#ifdef HAVE_JPEG
#include "jpegdrv.h"
#endif

#ifdef HAVE_FFMPEG
#include "ffmpegdrv.h"
#endif

#ifdef HAVE_QUICKTIME
#include "quicktimedrv.h"
#endif

struct gfxoutputdrv_list_s {
    struct gfxoutputdrv_s *drv;
    struct gfxoutputdrv_list_s *next;
};
typedef struct gfxoutputdrv_list_s gfxoutputdrv_list_t;

static gfxoutputdrv_list_t *gfxoutputdrv_list = NULL;
static int gfxoutputdrv_list_count = 0;
static log_t gfxoutput_log = LOG_ERR;
static gfxoutputdrv_list_t *gfxoutputdrv_list_iter = NULL;

int gfxoutput_num_drivers(void)
{
    return 0;
}

gfxoutputdrv_t *gfxoutput_drivers_iter_init(void)
{
    return NULL;
}

gfxoutputdrv_t *gfxoutput_drivers_iter_next(void)
{
    return NULL;
}
#endif
int gfxoutput_early_init(int help)
{
    return 0;
}

int gfxoutput_init(void)
{
    return 0;
}

void gfxoutput_shutdown(void)
{
}

/*-----------------------------------------------------------------------*/

int gfxoutput_register(gfxoutputdrv_t *drv)
{
    return 0;
}

gfxoutputdrv_t *gfxoutput_get_driver(const char *drvname)
{
    return NULL;
}

int gfxoutput_resources_init(void)
{
    return 0;
}

int gfxoutput_cmdline_options_init(void)
{
    return 0;
}
