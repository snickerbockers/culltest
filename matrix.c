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
#include <kos.h>

#include "matrix.h"

void perspective_mat(float ret_mat[16], float fovy, float aspect,
                     float z_near, float z_far) {
    float bottom = sin(fovy * (M_PI / 180.0) * 0.5) * z_near;
    float right = bottom * aspect;

    ret_mat[0] = z_near / right;
    ret_mat[1] = 0.0f;
    ret_mat[2] = 0.0f;
    ret_mat[3] = 0.0f;

    ret_mat[4] = 0.0f;
    ret_mat[5] = z_near / bottom;
    ret_mat[6] = 0.0f;
    ret_mat[7] = 0.0f;

    ret_mat[8] = 0.0f;
    ret_mat[9] = 0.0f;
    ret_mat[10] = -(z_far + z_near) / (z_far - z_near);
    ret_mat[11] = -1.0f;

    ret_mat[12] = 0.0f;
    ret_mat[13] = 0.0f;
    ret_mat[14] = -2.0 * z_far * z_near / (z_far - z_near);
    ret_mat[15] = 0.0f;
}

void
mat_mult(float mat_out[16], float const lhs[16], float const rhs[16]) {
    unsigned lhs_row, rhs_col, idx;
    for (lhs_row = 0; lhs_row < 4; lhs_row++) {
        for (rhs_col = 0; rhs_col < 4; rhs_col++) {
            float sum = 0;
            for (idx = 0; idx < 4; idx++)
                sum += lhs[lhs_row * 4 + idx] * rhs[idx * 4 + rhs_col];
            mat_out[lhs_row * 4 + rhs_col] = sum;
        }
    }
}

void ident_mat(float mat[16]) {
    mat[0]  = 1.0f; mat[1]  = 0.0f; mat[2]  = 0.0f; mat[3]  = 0.0f;
    mat[4]  = 0.0f; mat[5]  = 1.0f; mat[6]  = 0.0f; mat[7]  = 0.0f;
    mat[8]  = 0.0f; mat[9]  = 0.0f; mat[10] = 1.0f; mat[11] = 0.0f;
    mat[12] = 0.0f; mat[13] = 0.0f; mat[14] = 0.0f; mat[15] = 1.0f;
}

void trans_mat(float mat[16], float const disp[3]) {
    mat[0]  = 1.0f; mat[1]  = 0.0f; mat[2]  = 0.0f; mat[3]  = disp[0];
    mat[4]  = 0.0f; mat[5]  = 1.0f; mat[6]  = 0.0f; mat[7]  = disp[1];
    mat[8]  = 0.0f; mat[9]  = 0.0f; mat[10] = 1.0f; mat[11] = disp[2];
    mat[12] = 0.0f; mat[13] = 0.0f; mat[14] = 0.0f; mat[15] = 1.0f;
}

static inline float dot4(float const v1[4], float const v2[4]) {
    return v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2] + v1[3] * v2[3];
}

void
mat_vec_mult(float vec_out[4],
             float const mat[16], float const vec_in[4]) {
    vec_out[0] = dot4(mat, vec_in);
    vec_out[1] = dot4(mat + 4, vec_in);
    vec_out[2] = dot4(mat + 8, vec_in);
    vec_out[3] = dot4(mat + 12, vec_in);
}

void pitch_mat(float mat[16], float radians) {
    float cosine = cosf(radians);
    float sine = sinf(radians);
    mat[0] = 1.0f;  mat[1] = 0.0f;   mat[2] = 0.0f;    mat[3] = 0.0f;
    mat[4] = 0.0f;  mat[5] = cosine; mat[6] = -sine;   mat[7] = 0.0f;
    mat[8] = 0.0f;  mat[9] = sine;   mat[10] = cosine; mat[11] = 0.0f;
    mat[12] = 0.0f; mat[13] = 0.0f;  mat[14] = 0.0f;   mat[15] = 1.0f;
}

void yaw_mat(float mat[16], float radians) {
    float cosine = cosf(radians);
    float sine = sinf(radians);
    mat[0] = cosine; mat[1] = 0.0f;  mat[2] = sine;    mat[3] = 0.0f;
    mat[4] = 0.0f;   mat[5] = 1.0f;  mat[6] = 0.0f;    mat[7] = 0.0f;
    mat[8] = -sine;  mat[9] = 0.0f;  mat[10] = cosine; mat[11] = 0.0f;
    mat[12] = 0.0f;  mat[13] = 0.0f; mat[14] = 0.0f;   mat[15] = 1.0f;
}

void print_mat(float const mat[16]) {
    int row;
    for (row = 0; row < 4; row++) {
        printf("\t| %f %f %f %f |\n",
               (double)mat[row * 4], (double)mat[row * 4 + 1],
               (double)mat[row * 4 + 2], (double)mat[row * 4 + 3]);
    }
}
