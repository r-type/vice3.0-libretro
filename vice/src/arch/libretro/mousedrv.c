/*
 * mousedrv.c - Mouse handling for SDL.
 *
 * Written by
 *  Hannu Nuotio <hannu.nuotio@tut.fi>
 *
 * Based on code by
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  Oliver Schaertel <orschaer@forwiss.uni-erlangen.de>
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include "vice.h"

#include "mouse.h"
#include "mousedrv.h"
#include "ui.h"
#include "vsyncapi.h"

extern unsigned int opt_mouse_speed;

int mouse_x, mouse_y;
int mouse_accelx = 1, mouse_accely = 1;
static unsigned long mouse_timestamp = 0;
static mouse_func_t mouse_funcs;

void mousedrv_mouse_changed(void)
{
    ui_check_mouse_cursor();
}

int mousedrv_resources_init(mouse_func_t *funcs)
{
    mouse_funcs.mbl = funcs->mbl;
    mouse_funcs.mbr = funcs->mbr;
    mouse_funcs.mbm = funcs->mbm;
    mouse_funcs.mbu = funcs->mbu;
    mouse_funcs.mbd = funcs->mbd;
    return 0;
}

/* ------------------------------------------------------------------------- */

int mousedrv_cmdline_options_init(void)
{
    return 0;
}

/* ------------------------------------------------------------------------- */

void mousedrv_init(void)
{
}

/* ------------------------------------------------------------------------- */

void mouse_button_left(int pressed)
{
    mouse_funcs.mbl(pressed);
}

void mouse_button_right(int pressed)
{
    mouse_funcs.mbr(pressed);
}

void mouse_button_middle(int pressed)
{
    mouse_funcs.mbm(pressed);
}

void mouse_button(int bnumber, int state)
{
    switch (bnumber) {
        case 0:
            mouse_button_left(state);
            break;
        case 1:
            mouse_button_right(state);
            break;
        case 2:
            mouse_button_middle(state);
            break;
        default:
            break;
    }
}

int mousedrv_get_x(void)
{
    return mouse_x;
}

int mousedrv_get_y(void)
{
    return mouse_y;
}

void mouse_move(int x, int y)
{
#if 0
    printf("x:%3d y:%3d, mx:%5d my:%5d\n", x, y, mouse_x, mouse_y);
#endif
    int x_new = (x * (float)opt_mouse_speed) / 100.0f;
    int y_new = (y * (float)opt_mouse_speed) / 100.0f;

    if (abs(x_new) < 1 && x != 0)
        x_new = (x < 0) ? -1 : 1;
    if (abs(y_new) < 1 && y != 0)
        y_new = (y < 0) ? -1 : 1;

    mouse_x += x_new;
    mouse_y -= y_new;

    mouse_timestamp = vsyncarch_gettime() * 100;
#if 0
    printf("             mx:%5d my:%5d, x_new:%4d y_new:%4d, ts:%d\n", mouse_x, mouse_y, x_new, y_new, mouse_timestamp);
#endif
}

unsigned long mousedrv_get_timestamp(void)
{
    return mouse_timestamp;
}
