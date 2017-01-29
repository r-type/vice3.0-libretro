/*
 * sounddummy.c - Implementation of the dummy sound device
 *
 * Written by
 *  Teemu Rantanen <tvr@cs.hut.fi>
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

#include "sound.h"

extern void retro_audiocb(signed short int *sound_buffer,int sndbufsize);

static int retro_sound_init(const char *param, int *speed,
		    int *fragsize, int *fragnr, int *channels)
{
  *speed = 44100;
  *fragsize = (*speed)/50;
  *channels = 1;

  int buffer_size = (*fragnr) * (*channels) * (*fragsize) + 1;

  return 0;
}
static int retro_write(SWORD *pbuf, size_t nr)
{
//printf("nr:%d ",nr);
retro_audiocb(pbuf, nr);
    return 0;
}


static int retro_flush(char *state){
//printf("flush\n");
return 0;
}

static sound_device_t retro_device =
{
    "retro",
    retro_sound_init,
    retro_write,
    NULL,
    retro_flush,
    NULL,
    NULL,
    NULL,
    NULL,
    0,
    1
};

int sound_init_retro_device(void)
{
    return sound_register_device(&retro_device);
}
