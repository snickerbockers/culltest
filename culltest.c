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

#include <kos.h>
#include <dc/pvr.h>
#include <dc/biosfont.h>
#include <dc/matrix3d.h>

#include "romfont.h"

static pvr_init_params_t pvr_params = {
    { PVR_BINSIZE_32, PVR_BINSIZE_0,
      PVR_BINSIZE_0, PVR_BINSIZE_0, PVR_BINSIZE_0 },
    64 * 1024,0,0,1
};

static void perspective_mat(float ret_mat[16], float fovy, float aspect,
                            float z_near, float z_far) {
    float bottom = fsin(fovy * 0.5f) * z_near;
    float right = bottom * aspect;
    float far_minus_near_recip = 1.0f / (z_far - z_near);

    ret_mat[0] = z_near / right;
    ret_mat[4] = 0.0f;
    ret_mat[8] = 0.0f;
    ret_mat[12] = 0.0f;

    ret_mat[1] = 0.0f;
    ret_mat[5] = z_near / bottom;
    ret_mat[9] = 0.0f;
    ret_mat[13] = 0.0f;

    ret_mat[2] = 0.0f;
    ret_mat[6] = 0.0f;
    ret_mat[10] = -(z_far + z_near) * far_minus_near_recip;
    ret_mat[14] = -1.0f;

    ret_mat[3] = 0.0f;
    ret_mat[7] = 0.0f;
    ret_mat[11] = -2.0 * z_far * z_near * far_minus_near_recip;
    ret_mat[15] = 0.0f;
}

static void
mat_mult(float mat_out[16], float const lhs[16], float const rhs[16]) {
    mat_out[0] = lhs[0] * rhs[0] + lhs[1] * rhs[4] +
        lhs[2] * rhs[8] + lhs[3] * rhs[12];
    mat_out[1] = lhs[0] * rhs[1] + lhs[1] * rhs[5] +
        lhs[2] * rhs[9] + lhs[3] * rhs[13];
    mat_out[2] = lhs[0] * rhs[2] + lhs[1] * rhs[6] +
        lhs[2] * rhs[10] + lhs[3] * rhs[14];
    mat_out[3] = lhs[0] * rhs[3] + lhs[1] * rhs[7] +
        lhs[2] * rhs[14] + lhs[3] * rhs[15];
    mat_out[4] = lhs[4] * rhs[0] + lhs[5] * rhs[4] +
        lhs[6] * rhs[8] + lhs[7] * rhs[12];
    mat_out[5] = lhs[4] * rhs[1] + lhs[5] * rhs[5] +
        lhs[6] * rhs[9] + lhs[7] * rhs[13];
    mat_out[6] = lhs[4] * rhs[2] + lhs[5] * rhs[6] +
        lhs[6] * rhs[10] + lhs[7] * rhs[14];
    mat_out[7] = lhs[4] * rhs[3] + lhs[5] * rhs[7] +
        lhs[6] * rhs[14] + lhs[7] * rhs[15];
    mat_out[8] = lhs[8] * rhs[0] + lhs[9] * rhs[4] +
        lhs[10] * rhs[8] + lhs[11] * rhs[12];
    mat_out[9] = lhs[8] * rhs[1] + lhs[9] * rhs[5] +
        lhs[10] * rhs[9] + lhs[11] * rhs[13];
    mat_out[10] = lhs[8] * rhs[2] + lhs[9] * rhs[6] +
        lhs[10] * rhs[10] + lhs[11] * rhs[14];
    mat_out[11] = lhs[8] * rhs[3] + lhs[9] * rhs[7] +
        lhs[10] * rhs[14] + lhs[11] * rhs[15];
    mat_out[12] = lhs[12] * rhs[0] + lhs[13] * rhs[4] +
        lhs[14] * rhs[8] + lhs[15] * rhs[12];
    mat_out[13] = lhs[12] * rhs[1] + lhs[13] * rhs[5] +
        lhs[14] * rhs[9] + lhs[15] * rhs[13];
    mat_out[14] = lhs[12] * rhs[2] + lhs[13] * rhs[6] +
        lhs[14] * rhs[10] + lhs[15] * rhs[14];
    mat_out[15] = lhs[12] * rhs[3] + lhs[13] * rhs[7] +
        lhs[14] * rhs[14] + lhs[15] * rhs[15];
}

static void ident_mat(float mat[16]) {
    mat[0]  = 1.0f; mat[1]  = 0.0f; mat[2]  = 0.0f; mat[3]  = 0.0f;
    mat[4]  = 0.0f; mat[5]  = 1.0f; mat[6]  = 0.0f; mat[7]  = 0.0f;
    mat[8]  = 0.0f; mat[9]  = 0.0f; mat[10] = 1.0f; mat[11] = 0.0f;
    mat[12] = 0.0f; mat[13] = 0.0f; mat[14] = 0.0f; mat[15] = 1.0f;
}

static void trans_mat(float mat[16], float const disp[3]) {
    mat[0]  = 1.0f; mat[1]  = 0.0f; mat[2]  = 0.0f; mat[3]  = disp[0];
    mat[4]  = 0.0f; mat[5]  = 1.0f; mat[6]  = 0.0f; mat[7]  = disp[1];
    mat[8]  = 0.0f; mat[9]  = 0.0f; mat[10] = 1.0f; mat[11] = disp[2];
    mat[12] = 0.0f; mat[13] = 0.0f; mat[14] = 0.0f; mat[15] = 1.0f;
}

static inline float dot4(float const v1[4], float const v2[4]) {
    return v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2] + v1[3] * v2[3];
}

static void
transform_vec(float vec_out[4],
              float const mat[16], float const vec_in[4]) {
    vec_out[0] = dot4(mat, vec_in);
    vec_out[1] = dot4(mat + 4, vec_in);
    vec_out[2] = dot4(mat + 8, vec_in);
    vec_out[3] = dot4(mat + 12, vec_in);
}

int main(int argc, char **argv) {
    pvr_init(&pvr_params);

    pvr_vertex_t verts[] = {
        {
            .flags = PVR_CMD_VERTEX,
            /* .x = 0.0f, .y = 320.0f, .z = 10.0f, */
            .argb = 0xffff0000
        },
        {
            .flags = PVR_CMD_VERTEX,
            /* .x = 320.0f, .y = 0, .z = 10.0f, */
            .argb = 0xff00ff00
        },
        {
            .flags = PVR_CMD_VERTEX,
            /* .x = 640, .y = 320.0f, .z = 10.0f, */
            .argb = 0xff0000ff
        },
        {
            .flags = PVR_CMD_VERTEX_EOL,
            .argb = 0xffffffff//, /* .x = 640.0f,  */.y=0.0f, .z=10.0f
        }
    };

    static unsigned short font[288 * 24 * 12];
    create_font(font, ~0, 0);
    pvr_ptr_t tex = font_tex_create(font);

    pvr_poly_cxt_t poly_ctxt;
    pvr_poly_hdr_t poly_hdr;
    pvr_poly_cxt_col(&poly_ctxt, PVR_LIST_OP_POLY);
    pvr_poly_compile(&poly_hdr, &poly_ctxt);

    float mat[16], proj[16], trans[16];
    float disp[4] = { 320.0f, 240.0f, 0.0f, 1.0f };
    trans_mat(trans, disp);
    perspective_mat(proj, 90.0f, 640.0f / 480.0f, 0.1f, 10.0f);
    mat_mult(mat, trans, proj);
    /* ident_mat(mat); */
    float points_orig[4][4] = {
        { -320.0f, 0.0f, 2.0f, 1.0f },
        { 0.0f, -320.0f, 2.0f, 1.0f },
        { 320.0f, 0.0f, 2.0f, 1.0f },
        { -320.0f, -320.0f, 2.0f, 1.0f }
    };

    float translation[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

    int last_vbl_count = -1;
    printf("done initializing\n");
    for (;;) {
        bool up = false, down = false;

        maple_device_t *cont = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);
        if (cont) {
            cont_state_t *stat = maple_dev_status(cont);
            if (stat->buttons & CONT_START) {
                printf("user requested exit by pressing start on controller 0\n");
                break;
            }
            if (stat->buttons & CONT_DPAD_UP)
                up = true;
            if (stat->buttons & CONT_DPAD_DOWN)
                down = true;
        }

        int vbl_count = pvr_get_vbl_count();
        if (last_vbl_count != vbl_count) {
            int idx;
            last_vbl_count = vbl_count;

            float zdelta = 0.0f;
            if (up)
                zdelta += 1.0f / 30.0f;
            if (down)
                zdelta -= 1.0f / 30.0f;

            translation[2] += zdelta;

            trans_mat(trans, disp);
            perspective_mat(proj, 90.0f, 640.0f / 480.0f, 0.1f, 10.0f);
            mat_mult(mat, trans, proj);

            memcpy(proj, mat, sizeof(proj));
            ident_mat(trans);

            trans_mat(trans, translation);

            float points_final[4][4];
            for (idx = 0; idx < 4; idx++) {
                float tmpvec[4];
                /* transform_vec(points_final[idx], mat, points_orig[idx]); */
                transform_vec(tmpvec, trans, points_orig[idx]);
                transform_vec(points_final[idx], proj, tmpvec);
                points_final[idx][0] /= points_final[idx][3];
                points_final[idx][1] /= points_final[idx][3];
                points_final[idx][2] /= points_final[idx][3];
                points_final[idx][3] /= points_final[idx][3];

                verts[idx].x = points_final[idx][0];
                verts[idx].y = points_final[idx][1];
                verts[idx].z = points_final[idx][2];

                printf("{ %f, %f, %f }\n", (double)verts[idx].x, (double)verts[idx].y, (double)verts[idx].z );
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
