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

#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <kos.h>
#include <dc/pvr.h>
#include <dc/biosfont.h>
#include <dc/matrix3d.h>

#include "matrix.h"
#include "romfont.h"

static pvr_init_params_t pvr_params = {
    { PVR_BINSIZE_32, PVR_BINSIZE_0,
      PVR_BINSIZE_0, PVR_BINSIZE_0, PVR_BINSIZE_0 },
    64 * 1024,0,0,1
};

static void init_proj_mat(float proj[16]) {
    float offset[4] = { 320.0f, 240.0f, 0.0f, 1.0f };
    float offset_mat[16], persp_mat[16];
    trans_mat(offset_mat, offset);
    perspective_mat(persp_mat, 90.0f, 640.0f / 480.0f, 0.1f, 10.0f);
    mat_mult(proj, offset_mat, persp_mat);
}

static char const *itoa(int val) {
    static char buf[32];

    memset(buf, 0, sizeof(buf));

    int buflen = 0;
    if (val < 0) {
        buf[0] = '-';
        buflen++;
        val = -val;
    } else if (val == 0) {
        return "0";
    }

    bool leading = true;
    val &= 0xffffffff; // in case sizeof(int) > 4
    int power = 1000000000; // billion
    while (power > 0) {
        if (buflen >= sizeof(buf)) {
            buf[sizeof(buf) - 1] = '\0';
            break;
        }
        int digit = val / power;
        if (!(digit == 0 && leading))
            buf[buflen++] = digit + '0';
        if (digit != 0)
            leading = false;
        val = val % power;
        power /= 10;
    }

    return buf;
}

int main(int argc, char **argv) {
    pvr_init(&pvr_params);

    pvr_vertex_t verts[] = {
        {
            .flags = PVR_CMD_VERTEX,
            .argb = 0xffff0000
        },
        {
            .flags = PVR_CMD_VERTEX,
            .argb = 0xff00ff00
        },
        {
            .flags = PVR_CMD_VERTEX,
            .argb = 0xff0000ff
        },
        {
            .flags = PVR_CMD_VERTEX_EOL,
            .argb = 0xffffffff
        }
    };

    static unsigned short font[288 * 24 * 12];
    create_font(font, ~0, 0);
    pvr_ptr_t tex = font_tex_create(font);

    pvr_poly_cxt_t poly_ctxt;
    pvr_poly_hdr_t poly_hdr;
    pvr_poly_cxt_col(&poly_ctxt, PVR_LIST_OP_POLY);
    pvr_poly_compile(&poly_hdr, &poly_ctxt);

    float projection[16];
    init_proj_mat(projection);

    float mesh[4][4] = {
        { -320.0f, 0.0f, 0.0f, 1.0f },
        { 0.0f, -320.0f, 0.0f, 1.0f },
        { 320.0f, 0.0f, 0.0f, 1.0f },
        { -320.0f, -320.0f, 0.0f, 1.0f }
    };

    float translation[4] = { 0.0f, 0.0f, 2.0f, 0.0f };

    int last_vbl_count = -1;
    printf("done initializing\n");
    for (;;) {
        bool btn_b = false, btn_a = false, up = false,
            down = false, left = false, right = false;

        maple_device_t *cont = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);
        if (cont) {
            cont_state_t *stat = maple_dev_status(cont);
            if (stat->buttons & CONT_START) {
                printf("user requested exit by pressing start on controller 0\n");
                break;
            }
            if (stat->buttons & CONT_B)
                btn_b = true;
            if (stat->buttons & CONT_A)
                btn_a = true;
            if (stat->buttons & CONT_DPAD_UP)
                up = true;
            if (stat->buttons & CONT_DPAD_DOWN)
                down = true;
            if (stat->buttons & CONT_DPAD_LEFT)
                left = true;
            if (stat->buttons & CONT_DPAD_RIGHT)
                right = true;
        }

        int vbl_count = pvr_get_vbl_count();
        if (last_vbl_count != vbl_count) {
            int idx;
            last_vbl_count = vbl_count;

            float delta[3] = { 0.0f, 0.0f, 0.0f };
            if (btn_b)
                delta[2] += 1.0f / 30.0f;
            if (btn_a)
                delta[2] -= 1.0f / 30.0f;

            if (left)
                delta[0] -= 32.0f;
            if (right)
                delta[0] += 32.0f;
            if (up)
                delta[1] -= 32.0f;
            if (down)
                delta[1] += 32.0f;

            translation[0] += delta[0];
            translation[1] += delta[1];
            translation[2] += delta[2];

            float mview_mat[16];
            trans_mat(mview_mat, translation);

            float mview_proj_mat[16];
            mat_mult(mview_proj_mat, projection, mview_mat);

            float points_final[4][4];
            for (idx = 0; idx < 4; idx++) {
                mat_vec_mult(points_final[idx], mview_proj_mat, mesh[idx]);
                float recip_w = 1.0f / points_final[idx][3];

                verts[idx].x = points_final[idx][0] * recip_w;
                verts[idx].y = points_final[idx][1] * recip_w;
                verts[idx].z = recip_w;
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

        font_tex_render_string(tex, "x: ", 0, 1);
        char const *x_str = itoa(translation[0]);
        font_tex_render_string(tex, x_str, 4, 1);

        font_tex_render_string(tex, "y: ", 0, 2);
        char const *y_str = itoa(translation[1]);
        font_tex_render_string(tex, y_str, 4, 2);

        font_tex_render_string(tex, "z: ", 0, 3);
        char const *z_str = itoa(translation[2]);
        font_tex_render_string(tex, z_str, 4, 3);

        pvr_list_finish();
        pvr_scene_finish();
    }

    printf("exiting via return from main\n");
    return 0;
}
