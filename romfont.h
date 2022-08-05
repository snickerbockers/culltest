/*******************************************************************************
 *
 *
 *    Copyright (C) 2022 snickerbockers
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
 ******************************************************************************/

#ifndef ROMFONT_H_
#define ROMFONT_H_

#define GLYPH_WIDTH 12
#define GLYPH_HEIGHT 24

#define FONT_TEX_WIDTH 256
#define FONT_TEX_HEIGHT 512
#define FONT_ROW_BYTES (FONT_TEX_WIDTH * sizeof(uint16_t))
#define FONT_SIZE_BYTES (FONT_ROW_BYTES * FONT_TEX_HEIGHT)

#define FONT_TEX_N_COL (FONT_TEX_WIDTH / GLYPH_WIDTH)
#define FONT_TEX_N_ROW (FONT_TEX_HEIGHT / GLYPH_HEIGHT)

#define N_GLYPH 288

void *get_romfont_pointer(void);

void glyph_uv(unsigned glyph, float uv[4]);

unsigned short make_color(unsigned red, unsigned green, unsigned blue);
void
create_font(unsigned short *font,
            unsigned short foreground, unsigned short background);
void blit_glyph(void volatile *fb, unsigned linestride_pixels,
                unsigned xclip, unsigned yclip,
                unsigned short const *font, unsigned glyph_no,
                unsigned x, unsigned y);
void blit_char(void volatile *fb, unsigned linestride_pixels,
                unsigned xclip, unsigned yclip,
               unsigned short const *font,
               char ch, unsigned row, unsigned col);
void blitstring(void volatile *fb, unsigned linestride_pixels,
                unsigned xclip, unsigned yclip,
                unsigned short const *font, char const *msg,
                unsigned row, unsigned col);

pvr_ptr_t font_tex_create(unsigned short const *font);
void font_tex_render_char(pvr_ptr_t tex, char ch, unsigned xpos, unsigned ypos);
void font_tex_render_string(pvr_ptr_t tex, char const *str,
                            unsigned xpos, unsigned ypos);

#endif
