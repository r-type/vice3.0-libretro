/*
 * joy.c - Joystick support for MS-DOS.
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

#include <stdio.h>

#include "cmdline.h"
#include "keyboard.h"
#include "joy.h"
#include "joystick.h"
#include "machine.h"
#include "resources.h"
#include "translate.h"
#include "types.h"


int retrojoy_enabled;
int retrojoy_init=0;

static int set_retrojoy(int val, void *param)
{
    retrojoy_enabled = val ? 1 : 0;

    return 0;
}

static const resource_int_t retrojoy_resources_int[] = {
    { "RetroJoy", 0, RES_EVENT_NO, NULL,
      &retrojoy_enabled, set_retrojoy, NULL },
    RESOURCE_INT_LIST_END
};


static const resource_int_t resources_int[] = {
    { NULL }
};

/* ------------------------------------------------------------------------- */

int joystick_arch_init_resources(void)
{

	if(resources_register_int(retrojoy_resources_int)<0)return -1;

	retrojoy_init=1;

    return resources_register_int(resources_int);
}

int joy_arch_resources_init(void)
{
	if(resources_register_int(retrojoy_resources_int)<0)return -1;

	retrojoy_init=1;

    return resources_register_int(resources_int);
   // return 0;
}
int joy_arch_cmdline_options_init(void)
{
    return 0;
}

int joystick_init_cmdline_options(void)
{
    return 0;
}

/* ------------------------------------------------------------------------- */

int joy_arch_set_device(int port, int new_dev)
{
/*
    if (new_dev < 0 || new_dev > JOYDEV_MAX) {
        return -1;
    }
*/
    return 0;
}

/* Initialize joystick support.  */
int joy_arch_init(void)
{
    return 0;
}

/* Update the `joystick_value' variables according to the joystick status.  */
void joystick_update(void)
{
}

void joystick_close(void)
{
    /* Nothing to do on MSDOS.  */
    return;
}

void kbd_initialize_numpad_joykeys(int* joykeys)
{
}

/* ------------------------------------------------------------------------- */
