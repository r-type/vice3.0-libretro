/*
 * input_gp2x.c
 *
 * Written by
 *  Mike Dawson <mike@gp2x.org>
 *  Mustafa 'GnoStiC' Tufan <mtufan@gmail.com>
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

#include "gp2xsys.h"
#include "joystick.h"
#include "kbdbuf.h"
#include "keyboard.h"
#include "vkeyboard_gp2x.h"
#include "joystick.h"
//#include "audio_gp2x.h"
#include "prefs_gp2x.h"
#include "ui.h"
#include "resources.h"
//#include "keycodes.h"

//usbjoystick
unsigned int cur_portusb1 = 2;
unsigned int cur_portusb2 = 2;

//gp2x joystick
unsigned int cur_port = 2;

unsigned int input_up = 0;
unsigned int input_down = 0;
unsigned int input_left = 0;
unsigned int input_right = 0;
unsigned int input_a = 0;
unsigned int input_b = 0;
unsigned int input_x = 0;
unsigned int input_y = 0;
unsigned int input_select = 0;
unsigned int input_start = 0;

int input_left_last;
int input_right_last;
unsigned int input_a_last;
unsigned int input_b_last;
unsigned int input_x_last;
extern unsigned char keybuffer[64];
int pos = 25;

void gp2x_poll_input(void)
{

}


