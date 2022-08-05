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

#include <kos.h>
#include <dc/pvr.h>
#include <dc/biosfont.h>

#include "romfont.h"

static pvr_init_params_t pvr_params = {
    { PVR_BINSIZE_32, PVR_BINSIZE_0,
      PVR_BINSIZE_0, PVR_BINSIZE_0, PVR_BINSIZE_0 },
    64 * 1024,0,0,1
};

int main(int argc, char **argv) {
    pvr_init(&pvr_params);

    pvr_vertex_t verts[] = {
        {
            .flags = PVR_CMD_VERTEX,
            .x = 0.0f, .y = 320.0f, .z = 10.0f,
            .argb = 0xffff0000
        },
        {
            .flags = PVR_CMD_VERTEX,
            .x = 320.0f, .y = 0, .z = 10.0f,
            .argb = 0xff00ff00
        },
        {
            .flags = PVR_CMD_VERTEX,
            .x = 640, .y = 320.0f, .z = 10.0f,
            .argb = 0xff0000ff
        },
        {
            .flags = PVR_CMD_VERTEX_EOL,
            .argb = 0xffffffff, /* .x = 640.0f,  */.y=0.0f, .z=10.0f
        }
    };

    static unsigned short font[288 * 24 * 12];
    create_font(font, ~0, 0);
    pvr_ptr_t tex = font_tex_create(font);

    pvr_poly_cxt_t poly_ctxt;
    pvr_poly_hdr_t poly_hdr;
    pvr_poly_cxt_col(&poly_ctxt, PVR_LIST_OP_POLY);
    pvr_poly_compile(&poly_hdr, &poly_ctxt);

    printf("done initializing\n");
    for (;;) {
        maple_device_t *cont = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);
        if (cont) {
            cont_state_t *stat = maple_dev_status(cont);
            if (stat->buttons & CONT_START) {
                printf("user requested exit by pressing start on controller 0\n");
                break;
            }
        }
        
        pvr_wait_ready();
        pvr_scene_begin();

        pvr_list_begin(PVR_LIST_OP_POLY);

        int idx;
        pvr_prim(&poly_hdr, sizeof(poly_hdr));
        for (idx = 0; idx < 4; idx++)
            pvr_prim(verts + idx, sizeof(verts[idx]));

        font_tex_render_string(tex, "culltest", 0, 0);

        pvr_list_finish();

        pvr_scene_finish();
    }

    printf("exiting via return from main\n");
    return 0;
}
