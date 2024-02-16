/* Copyright (c) 2020-2024, ellie/@ell1e & Spew3D Team (see AUTHORS.md).

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.

Alternatively, at your option, this file is offered under the Apache 2
license, see accompanied LICENSE.md.
*/

#ifndef SPEW3D_MATH3D_H_
#define SPEW3D_MATH3D_H_

#include <math.h>

typedef struct s3d_pos {
    s3dnum_t x, y, z;
} s3d_pos;

typedef struct s3d_rotation {
    s3dnum_t hori, verti, roll;
} s3d_rotation;

static inline void spew3d_math3d_add(
        s3d_pos *p, s3d_pos *p2
        ) {
    p->x += p2->x;
    p->y += p2->y;
    p->z += p2->z;
}

static inline s3dnum_t spew3d_math3d_dist(
        s3d_pos *p1, s3d_pos *p2
        ) {
    double x_exp = (p1->x - p2->x) * (p1->x - p2->x);
    double y_exp = (p1->y - p2->y) * (p1->y - p2->y);
    double z_exp = (p1->z - p2->z) * (p1->z - p2->z);
    return sqrt(x_exp + y_exp + z_exp);
}

static inline s3dnum_t spew3d_math3d_upperbounddist(
        s3d_pos *p1, s3d_pos *p2
        ) {
    double result = fmax(fabs(p1->x - p2->x),
        fabs(p1->y - p2->y));
    result = fmax(result, fabs(p1->z - p2->z));
    return result;
}

static inline void spew3d_math3d_rotate(
        s3d_pos *p, s3d_rotation *r
        ) {
    /// Rotate a given pos around its origin by the given degrees.
    /// Positive angle gives CW (clockwise) rotation.
    /// X is forward (into screen), Y is left, Z is up.

    double roth = (r->hori * M_PI / 180.0);
    double rotv = (r->verti * M_PI / 180.0);
    double rotr = (r->roll * M_PI / 180.0);
    double newx, newy, newz;
    double px = p->x;
    double py = p->y;
    double pz = p->z;

    // Roll angle:
    newy = py * cos(rotr) + pz * sin(rotr);
    newz = pz * cos(rotr) - py * sin(rotr);
    p->z = newz;
    p->y = newy;

    py = newy;
    pz = newz;

    // Vertical angle:
    newz = pz * cos(rotv) + px * sin(rotv);
    newx = px * cos(rotv) - pz * sin(rotv);
    p->x = newx;
    p->z = newz;

    px = newx;
    pz = newz;

    // Horizontal angle:
    newy = py * cos(roth) + px * sin(roth);
    newx = px * cos(roth) - py * sin(roth);
    p->x = newx;
    p->y = newy;
}

#endif  // SPEW3D_MATH3D_H_
