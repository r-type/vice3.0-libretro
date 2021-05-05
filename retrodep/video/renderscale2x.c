/*
 * renderscale2x.c - scale2x renderers
 *
 * Written by
 *  Andreas Matthies <andreas.matthies@gmx.net>
 * Original Scale2x code by
 *  Andrea Mazzoleni <amadvance@users.sourceforge.net>
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

#include "renderscale2x.h"
#include "types.h"

static uint32_t scale2x(const uint32_t *colortab, const uint8_t **srcx1,
                     const uint8_t **srcx2, const uint8_t **srcy1,
                     const uint8_t **srcy2, const uint8_t **srce)
{
    return 0;
}


void render_08_scale2x(const video_render_color_tables_t *color_tab,
                       const uint8_t *src, uint8_t *trg,
                       unsigned int width, const unsigned int height,
                       const unsigned int xs, const unsigned int ys,
                       const unsigned int xt, const unsigned int yt,
                       const unsigned int pitchs, const unsigned int pitcht)
{
}


void render_16_scale2x(const video_render_color_tables_t *color_tab,
                       const uint8_t *src, uint8_t *trg,
                       unsigned int width, const unsigned int height,
                       const unsigned int xs, const unsigned int ys,
                       const unsigned int xt, const unsigned int yt,
                       const unsigned int pitchs, const unsigned int pitcht)
{
}


void render_24_scale2x(const video_render_color_tables_t *color_tab,
                       const uint8_t *src, uint8_t *trg,
                       unsigned int width, const unsigned int height,
                       const unsigned int xs, const unsigned int ys,
                       const unsigned int xt, const unsigned int yt,
                       const unsigned int pitchs, const unsigned int pitcht)
{
}


void render_32_scale2x(const video_render_color_tables_t *color_tab,
                       const uint8_t *src, uint8_t *trg,
                       unsigned int width, const unsigned int height,
                       const unsigned int xs, const unsigned int ys,
                       const unsigned int xt, const unsigned int yt,
                       const unsigned int pitchs, const unsigned int pitcht)
{
}
