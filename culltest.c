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
    perspective_mat(proj, 80.0f, 640.0f / 480.0f, 1.0f, 5000.0f);
}

static void rot_mat(float mat[16], float pitch, float yaw, float roll) {
    float pitch_matrix[16], yaw_matrix[16], roll_matrix[16], rollyaw_matrix[16];

    pitch_mat(pitch_matrix, pitch);
    yaw_mat(yaw_matrix, yaw);
    roll_mat(roll_matrix, roll);

    mat_mult(rollyaw_matrix, roll_matrix, yaw_matrix);
    mat_mult(mat, rollyaw_matrix, pitch_matrix);
}

enum screen_mode {
    SCREEN_3D,
    SCREEN_INFO,

    N_SCREENS
};

enum option_selection {
    OPT_SEL_CULL_MODE,
    OPT_SEL_CULL_VAL,

    OPT_SEL_CULL_LEN
};

// number of characters that can fit in the screen
#define SCREEN_TEXT_WIDTH (640 / GLYPH_WIDTH)
#define SCREEN_TEXT_HEIGHT (480 / GLYPH_HEIGHT)

#define SCREEN_INFO_COL_0 (SCREEN_TEXT_WIDTH / 4)
#define SCREEN_INFO_COL_1 (2 * SCREEN_TEXT_WIDTH / 4)

int main(int argc, char **argv) {
    bool draw_extra_tri = false;
    enum screen_mode cur_screen = SCREEN_3D;

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

    static unsigned short white_font[288 * 24 * 12];
    static unsigned short blue_font[288 * 24 * 12];
    create_font(white_font, ~0, 0);
    create_font(blue_font, make_color(103, 113, 252), 0);
    pvr_ptr_t white_font_tex = font_tex_create(white_font);
    pvr_ptr_t blue_font_tex = font_tex_create(blue_font);

    float projection[16];
    init_proj_mat(projection);

    float mesh[4][4] = {
        { -1.0f, -1.0f, 0.0f, 1.0f },
        {  1.0f, -1.0f, 0.0f, 1.0f },
        { -1.0f,  1.0f, 0.0f, 1.0f },
        {  1.0f,  1.0f, 0.0f, 1.0f }
    };

    float translation[4] = { 0.0f, 0.0f, -2.0f, 0.0f };

    int last_vbl_count = -1;
    printf("done initializing\n");
    bool btn_y_prev = false, btn_x_prev = false, up_prev = false,
        down_prev = false, a_prev = false;
    float pitch = 0.0f, yaw = 0.0f, roll = 0.0f;
    int cull_mode = PVR_CULLING_NONE;
    enum option_selection opt_sel = OPT_SEL_CULL_MODE;
    float cull_tolerance = 0.0f;
    static char const* cull_mode_names[] = {
        "PVR_CULLING_NONE",
        "PVR_CULLING_SMALL",
        "PVR_CULLING_NEGATIVE",
        "PVR_CULLING_POSITIVE"
    };

    for (;;) {
        bool btn_b = false, btn_a = false, btn_y = false, up = false,
            down = false, left = false, right = false, btn_x = false;
        int joyx, joyy, ltrig, rtrig;

        maple_device_t *cont = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);
        if (cont) {
            cont_state_t *stat = maple_dev_status(cont);
            if (stat->buttons & CONT_START) {
                printf("**** start button pressed - exiting! ****\n");
                exit(0);
            }
            if (stat->buttons & CONT_B)
                btn_b = true;
            if (stat->buttons & CONT_A)
                btn_a = true;
            if (stat->buttons & CONT_Y)
                btn_y = true;
            if (stat->buttons & CONT_DPAD_UP)
                up = true;
            if (stat->buttons & CONT_DPAD_DOWN)
                down = true;
            if (stat->buttons & CONT_DPAD_LEFT)
                left = true;
            if (stat->buttons & CONT_DPAD_RIGHT)
                right = true;
            if (stat->buttons & CONT_X)
                btn_x = true;
            joyx = stat->joyx;
            joyy = stat->joyy;
            ltrig = stat->ltrig;
            rtrig = stat->rtrig;
        } else {
            joyx = 0;
            joyy = 0;
            ltrig = 0;
            rtrig = 0;
        }

        /*
         * determinant used for backface culling
         * we only need it for the info screen
         */
        float det[2] = { 0.0f, 0.0f };

        int vbl_count = pvr_get_vbl_count();
        if (last_vbl_count != vbl_count) {
            int idx;
            last_vbl_count = vbl_count;

            if (cur_screen == SCREEN_3D) {
#define DEADZONE 32.0f
                if (fabsf(joyy) > DEADZONE)
                    pitch -= joyy / (128.0f * 30.0f);
                if (fabsf(joyx) > DEADZONE)
                    yaw += joyx / (128.0f * 30.0f);

                if (fabsf(ltrig) > DEADZONE)
                    roll -= ltrig / (128.0f * 30.0f);
                if (fabsf(rtrig) > DEADZONE)
                    roll += rtrig / (128.0f * 30.0f);

                pitch = fmodf(pitch, 2.0f * M_PI);
                yaw = fmodf(yaw, 2.0f * M_PI);
                roll = fmodf(roll, 2.0f * M_PI);

                if (pitch < 0.0f)
                    pitch += 2.0f * M_PI;
                if (yaw < 0.0f)
                    yaw += 2.0f * M_PI;
                if (roll < 0.0f)
                    roll += 2.0f * M_PI;

                float delta[3] = { 0.0f, 0.0f, 0.0f };
                if (btn_b)
                    delta[2] += 1.0f / 30.0f;
                if (btn_a)
                    delta[2] -= 1.0f / 30.0f;

                if (left)
                    delta[0] -= 1.0f/60.0f;
                if (right)
                    delta[0] += 1.0f/60.0f;
                if (up)
                    delta[1] -= 1.0f/60.0f;
                if (down)
                    delta[1] += 1.0f/60.0f;

                translation[0] += delta[0];
                translation[1] += delta[1];
                translation[2] += delta[2];
            } else if (cur_screen == SCREEN_INFO) {
                if (btn_a && !a_prev)
                    opt_sel = (opt_sel + 1) % OPT_SEL_CULL_LEN;

                if (opt_sel == OPT_SEL_CULL_MODE) {
                    if (up && !up_prev)
                        cull_mode = (cull_mode + 1) % 4;

                    if (down && !down_prev) {
                        cull_mode = cull_mode - 1;
                        if (cull_mode < 0)
                            cull_mode = 3;
                    }
                } else if (opt_sel == OPT_SEL_CULL_VAL) {
                    if (up && !up_prev)
                        cull_tolerance += 100.0f;
                    if (down && !down_prev)
                        cull_tolerance -= 100.0f;
                }
            }

            a_prev = btn_a;
            up_prev = up;
            down_prev = down;

            float mview_mat[16], rotation_mat[16], translation_mat[16];
            rot_mat(rotation_mat, pitch, yaw, roll);
            /* printf("rotation matrix:\n"); */
            /* print_mat(rotation_mat); */
            trans_mat(translation_mat, translation);
            mat_mult(mview_mat, translation_mat, rotation_mat);

            float mview_proj_mat[16];
            mat_mult(mview_proj_mat, projection, mview_mat);

            float points_final[4][4];
            for (idx = 0; idx < 4; idx++) {
                mat_vec_mult(points_final[idx], mview_proj_mat, mesh[idx]);
                float recip_w = 1.0f / points_final[idx][3];

                verts[idx].x = points_final[idx][0] * recip_w;
                verts[idx].y = points_final[idx][1] * recip_w;
                verts[idx].z = recip_w;

                verts[idx].x = (verts[idx].x + 1.0f) * 320.0f;
                verts[idx].y = (verts[idx].y + 1.0f) * 120.0f;
            }

            det[0] = verts[0].x * (verts[1].y - verts[2].y) +
                verts[1].x * (verts[2].y - verts[0].y) +
                verts[2].x * (verts[0].y - verts[1].y);


            det[1] = verts[1].x * (verts[2].y - verts[3].y) +
                verts[2].x * (verts[3].y - verts[1].y) +
                verts[3].x * (verts[1].y - verts[2].y);
        }

        if (btn_y && !btn_y_prev)
            draw_extra_tri = !draw_extra_tri;
        btn_y_prev = btn_y;

        if (btn_x && !btn_x_prev)
            cur_screen = (cur_screen + 1) % N_SCREENS;
        btn_x_prev = btn_x;

        int n_verts; // how many vertices to send to the GPU
        if (draw_extra_tri) {
            verts[2].flags = PVR_CMD_VERTEX;
            n_verts = 4;
        } else {
            verts[2].flags = PVR_CMD_VERTEX_EOL;
            n_verts = 3;
        }

        pvr_wait_ready();
        pvr_scene_begin();

        if (cur_screen == SCREEN_3D) {
            pvr_list_begin(PVR_LIST_OP_POLY);

            pvr_poly_cxt_t poly_ctxt;
            pvr_poly_hdr_t poly_hdr;
            pvr_poly_cxt_col(&poly_ctxt, PVR_LIST_OP_POLY);
            poly_ctxt.gen.culling = cull_mode;
            pvr_poly_compile(&poly_hdr, &poly_ctxt);

            /*
             * write cull_tolerance to the PVR_OBJECT_CLIP register
             *
             * ideally this would be done via KOS' PVR_SET macro; however
             * PVR_SET doesn't actually work here because it casts to
             * a uint32, so we have to do it ourselves.
             */
            /* PVR_SET(PVR_OBJECT_CLIP, cull_tolerance); */
            *((float volatile*)0xa05f8078) = cull_tolerance;

            int idx;
            pvr_prim(&poly_hdr, sizeof(poly_hdr));
            for (idx = 0; idx < n_verts; idx++)
                pvr_prim(verts + idx, sizeof(verts[idx]));

            int nextrow = 0;
            font_tex_render_string(white_font_tex, "culltest", 0, nextrow++);

            char tmpstr[64] = { 0 };
            snprintf(tmpstr, sizeof(tmpstr) - 1, "x: %.02f", (double)translation[0]);
            font_tex_render_string(white_font_tex, tmpstr, 0, nextrow++);

            snprintf(tmpstr, sizeof(tmpstr) - 1, "y: %.02f", (double)translation[1]);
            font_tex_render_string(white_font_tex, tmpstr, 0, nextrow++);

            snprintf(tmpstr, sizeof(tmpstr) - 1, "z: %.02f", (double)translation[2]);
            font_tex_render_string(white_font_tex, tmpstr, 0, nextrow++);

            snprintf(tmpstr, sizeof(tmpstr) - 1, "det[0]: %.02f\n", (double)det[0]);
            font_tex_render_string(white_font_tex, tmpstr, 0, nextrow++);
            if (draw_extra_tri) {
                snprintf(tmpstr, sizeof(tmpstr) - 1, "det[1]: %.02f\n",
                         (double)det[1]);
                font_tex_render_string(white_font_tex, tmpstr, 0, nextrow++);
            }

            pvr_list_finish();
        } else {
            pvr_list_begin(PVR_LIST_OP_POLY);

            char tmpstr[256] = { 0 };

            font_tex_render_string(white_font_tex, "STATUS",
                                   (SCREEN_TEXT_WIDTH - strlen("STATUS")) / 2, 1);

            int nextrow = 3;
            font_tex_render_string(white_font_tex, "v1:", SCREEN_INFO_COL_0, nextrow);
            snprintf(tmpstr, sizeof(tmpstr) - 1, "(%.02f, %.02f, %.02f)\n",
                     (double)verts[0].x, (double)verts[0].y, (double)verts[0].z);
            font_tex_render_string(white_font_tex, tmpstr, SCREEN_INFO_COL_1, nextrow++);

            font_tex_render_string(white_font_tex, "v2:", SCREEN_INFO_COL_0, nextrow);
            snprintf(tmpstr, sizeof(tmpstr) - 1, "(%.02f, %.02f, %.02f)\n",
                     (double)verts[1].x, (double)verts[1].y, (double)verts[1].z);
            font_tex_render_string(white_font_tex, tmpstr, SCREEN_INFO_COL_1, nextrow++);

            font_tex_render_string(white_font_tex, "v3:", SCREEN_INFO_COL_0, nextrow);
            snprintf(tmpstr, sizeof(tmpstr) - 1, "(%.02f, %.02f, %.02f)\n",
                     (double)verts[2].x, (double)verts[2].y, (double)verts[2].z);
            font_tex_render_string(white_font_tex, tmpstr, SCREEN_INFO_COL_1, nextrow++);

            if (draw_extra_tri) {
                font_tex_render_string(white_font_tex, "v4:", SCREEN_INFO_COL_0, nextrow);
                snprintf(tmpstr, sizeof(tmpstr) - 1, "(%.02f, %.02f, %.02f)\n",
                         (double)verts[3].x, (double)verts[3].y, (double)verts[3].z);
                font_tex_render_string(white_font_tex, tmpstr, SCREEN_INFO_COL_1, nextrow++);
            }

            font_tex_render_string(white_font_tex, "rot:", SCREEN_INFO_COL_0, nextrow);
            snprintf(tmpstr, sizeof(tmpstr) - 1, "(%.02f, %.02f, %.02f)\n",
                     (double)(pitch * 180.0f / M_PI),
                     (double)(yaw * 180.0f / M_PI),
                     (double)(roll * 180.0f / M_PI));
            font_tex_render_string(white_font_tex, tmpstr, SCREEN_INFO_COL_1, nextrow++);

            font_tex_render_string(white_font_tex, "det[0]:", SCREEN_INFO_COL_0, nextrow);
            snprintf(tmpstr, sizeof(tmpstr) - 1, "%.02f\n", (double)det[0]);
            font_tex_render_string(white_font_tex, tmpstr, SCREEN_INFO_COL_1, nextrow++);
            if (draw_extra_tri) {
                font_tex_render_string(white_font_tex, "det[1]:", SCREEN_INFO_COL_0, nextrow);
                snprintf(tmpstr, sizeof(tmpstr) - 1, "%.02f\n", (double)det[1]);
                font_tex_render_string(white_font_tex, tmpstr, SCREEN_INFO_COL_1, nextrow++);
            }

            pvr_ptr_t cull_mode_tex = (opt_sel == OPT_SEL_CULL_MODE ?
                                       blue_font_tex : white_font_tex);
            pvr_ptr_t cull_tolerance_tex = (opt_sel == OPT_SEL_CULL_VAL ?
                                            blue_font_tex : white_font_tex);

            font_tex_render_string(cull_mode_tex, "cull mode:",
                                   SCREEN_INFO_COL_0, nextrow);
            font_tex_render_string(cull_mode_tex,
                                   cull_mode_names[cull_mode % 4],
                                   SCREEN_INFO_COL_1, nextrow++);

            font_tex_render_string(cull_mode_tex, "cull bias:",
                                   SCREEN_INFO_COL_0, nextrow);
            snprintf(tmpstr, sizeof(tmpstr) - 1, "%.02f",
                     (double)cull_tolerance);
            font_tex_render_string(cull_tolerance_tex,
                                   tmpstr, SCREEN_INFO_COL_1, nextrow++);

            pvr_list_finish();
        }

        pvr_scene_finish();
    }

    printf("exiting via return from main\n");
    return 0;
}
