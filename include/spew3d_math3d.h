/* Copyright (c) 2020-2023, ellie/@ell1e & Spew3D Team (see AUTHORS.md).

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

typedef struct spew3d_pos {
    s3dnum_t x, y, z;
} spew3d_pos;

typedef struct spew3d_rotation {
    s3dnum_t hori, verti, roll;
} spew3d_rotation;


static inline void spew3d_math3d_add(
        spew3d_pos *p, spew3d_pos *p2
        ) {
    p->x += p2->x;
    p->y += p2->y;
    p->z += p2->z;
}

static inline void spew3d_math3d_rotate(
        spew3d_pos *p, spew3d_rotation *r
        ) {
    /// Rotate a given pos around its origin by the given degrees.
    /// Positive angle gives CW (clockwise) rotation.
    /// X is forward (into screen), Y is left, Z is up.

    double roth = ((double)r->hori / (double)S3D_METER /
        180.0) * M_PI;
    double rotv = ((double)r->verti / (double)S3D_METER /
        180.0) * M_PI;
    double rotr = ((double)r->roll / (double)S3D_METER /
        180.0) * M_PI;
    double newx, newy, newz;
    double px = (double)p->x / (double)S3D_METER;
    double py = (double)p->y / (double)S3D_METER;
    double pz = (double)p->z / (double)S3D_METER;

    // Roll angle:
    newy = (py) * cos(rotr) + (pz) * sin(rotr);
    newz = (pz) * cos(rotr) - (py) * sin(rotr);
    p->z = newz * S3D_METER;
    p->y = newy * S3D_METER;

    px = (double)p->x / (double)S3D_METER;
    py = (double)p->y / (double)S3D_METER;
    pz = (double)p->z / (double)S3D_METER;

    // Vertical angle:
    newz = (pz) * cos(rotv) + (px) * sin(rotv);
    newx = (px) * cos(rotv) - (pz) * sin(rotv);
    p->x = newx * S3D_METER;
    p->z = newz * S3D_METER;

    px = (double)p->x / (double)S3D_METER;
    py = (double)p->y / (double)S3D_METER;
    pz = (double)p->z / (double)S3D_METER;

    // Horizontal angle:
    newy = (py) * cos(roth) + (px) * sin(roth);
    newx = (px) * cos(roth) - (py) * sin(roth);
    p->x = newx * S3D_METER;
    p->y = newy * S3D_METER;
}

#endif  // SPEW3D_MATH3D_H_
