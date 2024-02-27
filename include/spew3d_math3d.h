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

/** Spew3D's default coordinate system for 3D
 *  =========================================
 *
 *  Positive angles describe CW (clockwise) rotations,
 *  or upward ones for vertical tilt.
 *  X is forward (into screen), Y is right, Z is up.
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

typedef struct s3d_color {
    s3dnum_t red, green, blue;
} s3d_color;

static inline void spew3d_math3d_add(
        s3d_pos *p, s3d_pos *p2
        ) {
    p->x += p2->x;
    p->y += p2->y;
    p->z += p2->z;
}

static inline void spew3d_math3d_sub(
        s3d_pos *p, s3d_pos *p2
        ) {
    p->x -= p2->x;
    p->y -= p2->y;
    p->z -= p2->z;
}

static inline void spew3d_math3d_scale(
        s3d_pos *p, double scale
        ) {
    p->x = scale * p->x;
    p->y = scale * p->y;
    p->z = scale * p->z;
}

static inline s3dnum_t spew3d_math3d_dist(
        s3d_pos *p1, s3d_pos *p2
        ) {
    double x_exp = (p1->x - p2->x) * (p1->x - p2->x);
    double y_exp = (p1->y - p2->y) * (p1->y - p2->y);
    double z_exp = (p1->z - p2->z) * (p1->z - p2->z);
    return sqrt(x_exp + y_exp + z_exp);
}

static inline s3d_pos spew3d_math3d_average(
        s3d_pos *v1, s3d_pos *v2
        ) {
    s3d_pos result;
    result.x = (v1->x + v2->x) / 2.0;
    result.y = (v1->y + v2->y) / 2.0;
    result.z = (v1->z + v2->z) / 2.0;
    return result;
}

static inline s3dnum_t spew3d_math3d_len(
        s3d_pos p
        ) {
    double x_exp = (0.0 - p.x) * (0.0 - p.x);
    double y_exp = (0.0 - p.y) * (0.0 - p.y);
    double z_exp = (0.0 - p.z) * (0.0 - p.z);
    return sqrt(x_exp + y_exp + z_exp);
}

static inline void spew3d_math3d_normalize(
        s3d_pos *v1
        ) {
    s3dnum_t l = spew3d_math3d_len(*v1);
    if (fabs(l) <= 0.0001) {
        v1->x = 0;
        v1->y = 0;
        v1->z = 0;
    } else {
        v1->x /= l;
        v1->y /= l;
        v1->z /= l;
    }
}

static inline s3dnum_t spew3d_math3d_flip(s3d_pos *p) {
    p->x = -p->x;
    p->y = -p->y;
    p->z = -p->z;
}

static inline s3dnum_t spew3d_math3d_upperbounddist(
        s3d_pos *p1, s3d_pos *p2
        ) {
    double result = fmax(fabs(p1->x - p2->x),
        fabs(p1->y - p2->y));
    result = fmax(result, fabs(p1->z - p2->z));
    return result;
}

typedef struct s3d_transform3d_cam_info {
    s3d_pos cam_pos;
    s3d_rotation cam_rotation;
    s3dnum_t cam_horifov;
    s3dnum_t cam_vertifov;
    uint32_t viewport_pixel_width;
    uint32_t viewport_pixel_height;

    int cache_set;
    double cached_screen_plane_x;
    double cached_screen_plane_ywidth;
    double cached_screen_plane_zheight;
    double cached_screen_plane_yleftoffset;
    double cached_screen_plane_pixel_ywidth;
    double cached_screen_plane_ztopoffset;
    double cached_screen_plane_pixel_zwidth;
    double cached_screen_plane_pixel_mixedwidth;
} s3d_transform3d_cam_info;


S3DEXP void spew3d_math3d_split_fovs_from_fov(
    s3dnum_t input_shared_fov,
    uint32_t pixel_width,
    uint32_t pixel_height,
    s3dnum_t *output_horifov,
    s3dnum_t *output_vertifov
);

S3DEXP void spew3d_math3d_cross_product(
    s3d_pos *v1, s3d_pos *v2, s3d_pos *out
);

S3DEXP void spew3d_math3d_polygon_normal(
    s3d_pos *v1, s3d_pos *v2, s3d_pos *v3,
    int do_normalize,
    s3d_pos *out
);

S3DEXP void spew3d_math3d_transform3d(
    s3d_pos input_pos,
    s3d_transform3d_cam_info *cam_info,
    s3d_pos model_world_pos,
    s3d_rotation model_world_rotation,
    s3d_pos *out_pos,
    s3d_pos *out_unscaled_pos
);

S3DEXP void spew3d_math3d_rotate(
    s3d_pos *p, s3d_rotation *r
);

S3DEXP s3dnum_t spew3d_math3d_anglefromto(
    s3d_pos *p, s3d_pos *p2
);

S3DEXP void spew3d_math3d_advanceforward(
        s3d_pos input_pos, double advance,
        s3d_rotation advance_dir,
        s3d_pos *out_pos
        ) {
    s3d_pos advance_vec = {0};
    advance_vec.x = advance;
    spew3d_math3d_rotate(
        &advance_vec, &advance_dir
    );
    out_pos->x = input_pos.x + advance_vec.x;
    out_pos->y = input_pos.y + advance_vec.y;
    out_pos->z = input_pos.z + advance_vec.z;
}

S3DEXP void spew3d_math3d_advancesideward(
        s3d_pos input_pos, double advance,
        s3d_rotation advance_dir,
        s3d_pos *out_pos
        ) {
    s3d_pos advance_vec = {0};
    advance_vec.y = advance;
    spew3d_math3d_rotate(
        &advance_vec, &advance_dir
    );
    out_pos->x = input_pos.x + advance_vec.x;
    out_pos->y = input_pos.y + advance_vec.y;
    out_pos->z = input_pos.z + advance_vec.z;
}

#endif  // SPEW3D_MATH3D_H_
