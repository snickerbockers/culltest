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

#include <stdint.h>

#include <kos.h>
#include <dc/pvr.h>

#include "romfont.h"

/* struct glyph_mapping { */
/*     unsigned glyph; */
/*     unsigned row, col; */
/* } const mappings[] = { */
/*     { '0', 0, 0 }, { '1', 0, 1 }, { '2', 0, 2 }, { '3', 0, 3 }, { '4', 0, 4 }, */
/*     { '5', 0, 5 }, { '6', 0, 6 }, { '7', 0, 7 }, { '8', 0, 8 }, { '9', 0, 9 }, */
/*     { 'a', 1, 0 }, { 'b', 1, 1 }, { 'c', 1, 2 }, { 'd', 1, 3 }, { 'e', 1, 4 }, */
/*     { 'f', 1, 5 }, { 'g', 1, 6 }, { 'h', 1, 7 }, { 'i', 1, 8 }, { 'j', 1, 9 }, */
/*     { 'k', 2, 0 }, { 'l', 2, 1 }, { 'm', 2, 2 }, { 'n', 2, 3 }, { 'o', 2, 4 }, */
/*     { 'p', 2, 5 }, { 'q', 2, 6 }, { 'r', 2, 7 }, { 's', 2, 8 }, { 't', 2, 9 }, */
/*     { 'u', 3, 0 }, { 'v', 3, 1 }, { 'w', 3, 2 }, { 'x', 3, 3 }, { 'y', 3, 4 }, */
/*     { 'z', 3, 5 }, { 'A', 3, 6 }, { 'B', 3, 7 }, { 'C', 3, 8 }, { 'D', 3, 9 }, */
/*     { 'E', 4, 0 }, { 'F', 4, 1 }, { 'G', 4, 2 }, { 'H', 4, 3 }, { 'I', 4, 4 }, */
/*     { 'J', 4, 5 }, { 'K', 4, 6 }, { 'L', 4, 7 }, { 'M', 4, 8 }, { 'N', 4, 9 }, */
/*     { 'O', 5, 0 }, { 'P', 5, 1 }, { 'Q', 5, 2 }, { 'R', 5, 3 }, { 'S', 5, 4 }, */
/*     { 'T', 5, 5 }, { 'U', 5, 6 }, { 'V', 5, 7 }, { 'W', 5, 8 }, { 'X', 5, 9 }, */
/*     { 'Y', 6, 0 }, { 'Z', 6, 1 }, */

/*     { '\0', 9, 9 } */
/* }; */

/* struct glyph_mapping const *find_glyph(int glyph) { */
/*     struct glyph_mapping const *curs = mappings; */
/*     while (curs->glyph && curs->glyph != glyph) */
/*         curs++; */
/*     return curs; */
/* } */

/* void glyph_uv(unsigned glyph, float uv[4]) { */
/*     struct glyph_mapping const *mapping = find_glyph(glyph); */
/*     uv[0] = mapping->col * GLYPH_WIDTH / (float)FONT_TEX_WIDTH; */
/*     uv[1] = mapping->row * GLYPH_HEIGHT / (float)FONT_TEX_HEIGHT; */
/*     uv[0] = (mapping->col + 1) * GLYPH_WIDTH / (float)FONT_TEX_WIDTH; */
/*     uv[1] = (mapping->row + 1) * GLYPH_HEIGHT / (float)FONT_TEX_HEIGHT; */
/* } */

void *get_romfont_pointer(void) {
    static uint32_t romfont_fn_ptr = 0x8c0000b4;
    void *romfont_p;
    asm volatile
        (
         "mov.l @%1, %1\n"
         "jsr @%1\n"
         "xor r1, r1\n"
         "mov r0, %0\n"
         : // output registers
           "=r"(romfont_p)
         : // input registers
           "r"(romfont_fn_ptr)
         : // clobbered registers
           "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "pr");
    return romfont_p;
}

/* static void */
/* render_glyph(unsigned char const *romfont, unsigned glyph, uint16_t *outp, unsigned linestride_pix) { */
/*     unsigned char const *glyph_in = romfont + (12 * 24 / 8) * glyph; */
/*     unsigned row, col; */
/*     for (row = 0; row < 24; row++) { */
/*         for (col = 0; col < 12; col++) { */
/*             unsigned idx = row * 12 + col; */
/*             unsigned char mask = 0x80 >> (idx % 8); */
/*             if (glyph_in[idx / 8] & mask) */
/*                 outp[col] = ~0; */
/*             else */
/*                 outp[col] = 0; */
/*         } */
/*         outp += linestride_pix; */
/*     } */
/* } */

// basic framebuffer parameters
#define LINESTRIDE_PIXELS  640
#define BYTES_PER_PIXEL      2
#define FRAMEBUFFER_WIDTH  640
#define FRAMEBUFFER_HEIGHT 476

unsigned short make_color(unsigned red, unsigned green, unsigned blue) {
    if (red > 255)
        red = 255;
    if (green > 255)
        green = 255;
    if (blue > 255)
        blue = 255;

    red >>= 3;
    green >>= 2;
    blue >>= 3;

    return blue | (green << 5) | (red << 11);
}

void
create_font(unsigned short *font,
            unsigned short foreground, unsigned short background) {
    char const *romfont = get_romfont_pointer();

    unsigned glyph;
    for (glyph = 0; glyph < 288; glyph++) {
        unsigned short *glyph_out = font + glyph * 24 * 12;
        char const *glyph_in = romfont + (12 * 24 / 8) * glyph;

        unsigned row, col;
        for (row = 0; row < 24; row++) {
            for (col = 0; col < 12; col++) {
                unsigned idx = row * 12 + col;
                char const *inp = glyph_in + idx / 8;
                char mask = 0x80 >> (idx % 8);
                unsigned short *outp = glyph_out + idx;
                if (*inp & mask)
                    *outp = foreground;
                else
                    *outp = background;
            }
        }
    }
}

void blit_glyph(void volatile *fb, unsigned linestride_pixels,
                unsigned xclip, unsigned yclip,
                unsigned short const *font,
                unsigned glyph_no, unsigned x, unsigned y) {
    if (glyph_no > 287)
        glyph_no = 0;
    unsigned short volatile *outp = ((unsigned short volatile*)fb) +
        y * linestride_pixels + x;
    unsigned short const *glyph = font + glyph_no * 24 * 12;

    unsigned row;
    for (row = 0; row < 24 && (row + y) < yclip; row++) {
        unsigned col;
        for (col = 0; col < 12 && (col + x) < xclip; col++) {
            outp[col] = glyph[row * 12 + col];
        }
        outp += linestride_pixels;
    }
}

void blit_char(void volatile *fb, unsigned linestride_pixels,
               unsigned xclip, unsigned yclip,
               unsigned short const *font,
               char ch, unsigned row, unsigned col) {
    /* if (row >= MAX_CHARS_Y || col >= MAX_CHARS_X) */
    /*     return; */

    unsigned x = col * 12;
    unsigned y = row * 24;

    unsigned glyph;
    if (ch >= 33 && ch <= 126)
        glyph = ch - 33 + 1;
    else
        return;

    blit_glyph(fb, linestride_pixels, xclip, yclip, font, glyph, x, y);
}

void blitstring(void volatile *fb, unsigned linestride_pixels,
                unsigned xclip, unsigned yclip,
                unsigned short const *font, char const *msg,
                unsigned row, unsigned col) {
    while (*msg) {
        /* if (col >= MAX_CHARS_X) { */
        /*     col = 0; */
        /*     row++; */
        /* } */
        if (*msg == '\n') {
            col = 0;
            row++;
            msg++;
            continue;
        }
        blit_char(fb, linestride_pixels, xclip, yclip, font, *msg++, row, col++);
    }
}

pvr_ptr_t font_tex_create(unsigned short const *font) {
    static uint16_t tmpbuf[FONT_TEX_WIDTH * FONT_TEX_HEIGHT];
    memset(tmpbuf, 0, sizeof(tmpbuf));
    unsigned glyph;
    unsigned col_next = 0, row_next = 0;
    for (glyph = 0; glyph < N_GLYPH; glyph++) {
        blit_glyph(tmpbuf, FONT_TEX_WIDTH,
                   FONT_TEX_WIDTH, FONT_TEX_HEIGHT, font, glyph,
                   GLYPH_WIDTH * col_next, GLYPH_HEIGHT * row_next);
        col_next++;
        if (col_next >= FONT_TEX_N_COL) {
            col_next = 0;
            row_next++;
        }
    }

    pvr_ptr_t tex = pvr_mem_malloc(sizeof(tmpbuf));
    pvr_txr_load(tmpbuf, tex, sizeof(tmpbuf));
    return tex;
}

static struct glyph_map {
    char ch;
    unsigned row, col;
} const mappings[] = {
    { '!', 0, 1 },
    { '"', 0, 2 },
    { '#', 0, 3 },
    { '$', 0, 4 },
    { '%', 0, 5 },
    { '&', 0, 6 },
    { '\'', 0, 7 },
    { '(', 0, 8 },
    { ')', 0, 9 },
    { '*', 0, 10 },
    { '+', 0, 11 },
    { ',', 0, 12 },
    { '-', 0, 13 },
    { '.', 0, 14 },
    { '/', 0, 15 },
    { '0', 0, 16 },
    { '1', 0, 17 },
    { '2', 0, 18 },
    { '3', 0, 19 },
    { '4', 0, 20 },

    { '5', 1, 0 },
    { '6', 1, 1 },
    { '7', 1, 2 },
    { '8', 1, 3 },
    { '9', 1, 4 },
    { ':', 1, 5 },
    { ';', 1, 6 },
    { '<', 1, 7 },
    { '=', 1, 8 },
    { '>', 1, 9 },
    { '?', 1, 10 },
    { '@', 1, 11 },
    { 'A', 1, 12 },
    { 'B', 1, 13 },
    { 'C', 1, 14 },
    { 'D', 1, 15 },
    { 'E', 1, 16 },
    { 'F', 1, 17 },
    { 'G', 1, 18 },
    { 'H', 1, 19 },
    { 'I', 1, 20 },

    { 'J', 2, 0 },
    { 'K', 2, 1 },
    { 'L', 2, 2 },
    { 'M', 2, 3 },
    { 'N', 2, 4 },
    { 'O', 2, 5 },
    { 'P', 2, 6 },
    { 'Q', 2, 7 },
    { 'R', 2, 8 },
    { 'S', 2, 9 },
    { 'T', 2, 10 },
    { 'U', 2, 11 },
    { 'V', 2, 12 },
    { 'W', 2, 13 },
    { 'X', 2, 14 },
    { 'Y', 2, 15 },
    { 'Z', 2, 16 },
    { '[', 2, 17 },
    { '\\', 2, 18 },
    { ']', 2, 19 },
    { '^', 2, 20 },

    { '_', 3, 0 },
    { '`', 3, 1 },
    { 'a', 3, 2 },
    { 'b', 3, 3 },
    { 'c', 3, 4 },
    { 'd', 3, 5 },
    { 'e', 3, 6 },
    { 'f', 3, 7 },
    { 'g', 3, 8 },
    { 'h', 3, 9 },
    { 'i', 3, 10 },
    { 'j', 3, 11 },
    { 'k', 3, 12 },
    { 'l', 3, 13 },
    { 'm', 3, 14 },
    { 'n', 3, 15 },
    { 'o', 3, 16 },
    { 'p', 3, 17 },
    { 'q', 3, 18 },
    { 'r', 3, 19 },
    { 's', 3, 20 },

    { 't', 4, 0 },
    { 'u', 4, 1 },
    { 'v', 4, 2 },
    { 'w', 4, 3 },
    { 'x', 4, 4 },
    { 'y', 4, 5 },
    { 'z', 4, 6 },
    { '{', 4, 7 },
    { '|', 4, 8 },
    { '}', 4, 9 },
    { '~', 4, 10 },

    /*
     * from this point on, the official dreamcast font only has
     * non-english characters
     */

    { ' ', FONT_TEX_N_ROW - 1, FONT_TEX_N_COL - 1 },

    { '\0', 0, 0 }
};

void font_tex_render_char(pvr_ptr_t tex, char ch, unsigned xpos, unsigned ypos) {
    struct glyph_map const *curs = mappings;
    while (curs->ch && curs->ch != ch) {
        curs++;
    }

    if (!curs->ch)
        return;

    float uv[4] = {
        curs->col * GLYPH_WIDTH / (float)FONT_TEX_WIDTH,
        curs->row * GLYPH_HEIGHT / (float)FONT_TEX_HEIGHT,
        (curs->col + 1) * GLYPH_WIDTH / (float)FONT_TEX_WIDTH,
        (curs->row + 1) * GLYPH_HEIGHT / (float)FONT_TEX_HEIGHT
    };

    float xy[4] = {
        xpos * GLYPH_WIDTH,
        ypos * GLYPH_HEIGHT,
        (xpos + 1) * GLYPH_WIDTH,
        (ypos + 1) * GLYPH_WIDTH
    };

    pvr_vertex_t quad[] = {
        {
            // lower-left
            .flags = PVR_CMD_VERTEX,
            .x = xy[0], .y = xy[3], .z = 10.0f,
            .u = uv[0], .v = uv[3],
            /* .argb = 0xffff0000 */
            .argb = 0xffffffff
        },
        {
            // lower-right
            .flags = PVR_CMD_VERTEX,
            .x = xy[2], .y = xy[3], .z = 10.0f,
            .u = uv[2], .v = uv[3],
            /* .argb = 0xff00ff00 */
            .argb = 0xffffffff
        },
        {
            // upper-left
            .flags = PVR_CMD_VERTEX,
            .x = xy[0], .y = xy[1], .z = 10.0f,
            .u = uv[0], .v = uv[1],
            /* .argb = 0xff0000ff */
            .argb = 0xffffffff
        },
        {
            // upper-right
            .flags = PVR_CMD_VERTEX_EOL,
            .x = xy[2], .y = xy[1], .z=10.0f,
            .u = uv[2], uv[1],
            .argb = 0xffffffff
        }
    };

    pvr_poly_cxt_t poly_ctxt;
    pvr_poly_hdr_t poly_hdr;

    pvr_poly_cxt_txr(&poly_ctxt, PVR_LIST_OP_POLY, PVR_TXRFMT_RGB565 | PVR_TXRFMT_NONTWIDDLED,
                     FONT_TEX_WIDTH, FONT_TEX_HEIGHT, tex, PVR_FILTER_BILINEAR);
    poly_ctxt.depth.comparison = PVR_DEPTHCMP_ALWAYS;
    pvr_poly_compile(&poly_hdr, &poly_ctxt);

    pvr_prim(&poly_hdr, sizeof(poly_hdr));
    int idx;
    for (idx = 0; idx < 4; idx++)
        pvr_prim(quad + idx, sizeof(quad[idx]));
}

void font_tex_render_string(pvr_ptr_t tex, char const *str,
                            unsigned xpos, unsigned ypos) {
    while (*str) {
        font_tex_render_char(tex, *str++, xpos++, ypos);
    }
}
