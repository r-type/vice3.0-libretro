/*
 * asm6809.c - 68090 Assembler-related utility functions.
 *
 * Written by
 *  Olaf Seibert <rhialto@falu.nl>
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

#include <stdlib.h>

#include "asm.h"
#include "mon_assemble.h"
#include "mon_register.h"
#include "montypes.h"
#include "types.h"

static const asm_opcode_info_t *asm_opcode_info_get(unsigned int p0, unsigned int p1, unsigned int p2)
{
    return 0;
}

static unsigned int asm_addr_mode_get_size(unsigned int mode, unsigned int p0,
                                           unsigned int p1, unsigned int p2)
{
    return 0;
}

void asm6809_init(monitor_cpu_type_t *monitor_cpu_type)
{
}
