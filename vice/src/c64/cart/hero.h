/*
 * hero.h - Cartridge handling, H.E.R.O.cart.
 *
 * Written by
 *  groepaz <groepaz@gmx.net>
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

#ifndef VICE_HERO_H
#define VICE_HERO_H

#include <stdio.h>

#include "types.h"

extern void hero_config_init(void);
extern void hero_config_setup(uint8_t *rawcart);
extern int hero_bin_attach(const char *filename, uint8_t *rawcart);
extern int hero_crt_attach(FILE *fd, uint8_t *rawcart);
extern void hero_detach(void);

struct snapshot_s;

extern int hero_snapshot_write_module(struct snapshot_s *s);
extern int hero_snapshot_read_module(struct snapshot_s *s);

#endif