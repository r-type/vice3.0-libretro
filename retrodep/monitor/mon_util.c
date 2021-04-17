/*
 * mon_util.c - Utilities for the VICE built-in monitor.
 *
 * Written by
 *  Spiro Trikaliotis <spiro.trikaliotis@gmx.de>
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
#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "archdep.h"
#include "console.h"
#include "lib.h"
#include "mem.h"
#include "mon_disassemble.h"
#include "mon_util.h"
#include "monitor.h"
#include "monitor_network.h"
#include "types.h"
#include "uimon.h"

static char *bigbuffer = NULL;
static const unsigned int bigbuffersize = 10000;
static unsigned int bigbufferwrite = 0;

static FILE *mon_log_file = NULL;

/******************************************************************************/

int mon_log_file_open(const char *name)
{
    return -1;
}

void mon_log_file_close(void)
{
}

static int mon_log_file_out(const char *buffer)
{
    return -1;
}

/******************************************************************************/

static void mon_buffer_alloc(void)
{
}

static int mon_buffer_flush(void)
{
    return 0;
}

/* like strncpy, but:
 * 1. always add a null character
 * 2. do not fill the rest of the buffer
 */
static void stringcopy_n(char *dest, const char *src, unsigned int len)
{
}

static void mon_buffer_add(const char *buffer, unsigned int bufferlen)
{
}

static int mon_out_buffered(const char *buffer)
{
    return 0;
}
#endif
int mon_out(const char *format, ...)
{
    return 0;
}
#if 0
char *mon_disassemble_with_label(MEMSPACE memspace, uint16_t loc, int hex,
                                 unsigned *opc_size_p, unsigned *label_p)
{
    return NULL;
}

char *mon_dump_with_label(MEMSPACE memspace, uint16_t loc, int hex, unsigned *label_p)
{
    return NULL;
}

#if !(defined(__OS2__) && !defined(USE_SDLUI))
static char *pchCommandLine = NULL;

void mon_set_command(console_t *cons_log, char *command,
                     void (*pAfter)(void))
{
}

char *uimon_in(const char *prompt)
{
    return NULL;
}
#endif

#endif
