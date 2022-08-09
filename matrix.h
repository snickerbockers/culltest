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

#ifndef MATRIX_H_
#define MATRIX_H_

void perspective_mat(float ret_mat[16], float fovy, float aspect,
                     float z_near, float z_far);

void
mat_mult(float mat_out[16], float const lhs[16], float const rhs[16]);

void ident_mat(float mat[16]);

void trans_mat(float mat[16], float const disp[3]);

void mat_vec_mult(float vec_out[4], float const mat[16], float const vec_in[4]);

#endif
