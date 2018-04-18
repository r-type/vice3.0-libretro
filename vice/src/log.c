/*
 * log.c - Logging facility. Overhauled for libretro use.
 *
 * Written by
 *  Ettore Perazzoli <ettore@comm2000.it>
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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libretro.h>

#include "archdep.h"
#include "cmdline.h"
#include "lib.h"
#include "log.h"
#include "resources.h"
#include "translate.h"
#include "util.h"

static int verbose = 0;
extern retro_log_printf_t log_cb;
static char log_buf[ 4096 ]; /*create this here in case of tiny stack*/

/* ------------------------------------------------------------------------- */

static int log_helper(unsigned int level, const char *format, va_list ap)
{
    int rc;

	rc = vsprintf( log_buf, format, ap );

	if ( rc >= 0 )
    {
		log_cb(level, "%s\n", log_buf);
		++rc;
	}

    return rc;
}

int log_message(log_t log, const char *format, ...)
{
    va_list ap;
    int rc;

    va_start(ap, format);
    rc = log_helper(RETRO_LOG_INFO, format, ap);
    va_end(ap);

    return rc;
}

int log_warning(log_t log, const char *format, ...)
{
    va_list ap;
    int rc;

    va_start(ap, format);
    rc = log_helper(RETRO_LOG_WARN, format, ap);
    va_end(ap);

    return rc;
}

int log_error(log_t log, const char *format, ...)
{
    va_list ap;
    int rc;

    va_start(ap, format);
    rc = log_helper(RETRO_LOG_ERROR, format, ap);
    va_end(ap);

    return rc;
}

int log_debug(const char *format, ...)
{
    va_list ap;
    int rc;

    va_start(ap, format);
    rc = log_helper(RETRO_LOG_DEBUG, format, ap);
    va_end(ap);

    return rc;
}

int log_verbose(const char *format, ...)
{
    va_list ap;
    int rc = 0;

    va_start(ap, format);
    if (verbose) {
        rc = log_helper(RETRO_LOG_INFO, format, ap);
    }
    va_end(ap);

    return rc;
}

/* ------------------------------------------------------------------------- */

/* stubs */
int log_resources_init(void) {return 0;}
void log_resources_shutdown(void) {}
int log_cmdline_options_init(void) {return 0;}
int log_init(void) {return 0;}
int log_init_with_fd(FILE *f) {return 0;}
log_t log_open(const char *id) {return 0;}
int log_close(log_t log) {return 0;}
void log_close_all(void) {}
void log_enable(int on) {}
int log_set_verbose(int n) {verbose=n?1:0;return 0;}
int log_verbose_init(int argc, char **argv) {return 0;}


