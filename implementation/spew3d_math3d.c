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

#ifdef SPEW3D_IMPLEMENTATION

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

S3DEXP void spew3d_math3d_split_fovs_from_fov(
        s3dnum_t input_shared_fov,
        uint32_t pixel_width,
        uint32_t pixel_height,
        s3dnum_t *output_horifov,
        s3dnum_t *output_vertifov
        ) {
    input_shared_fov = fmax(10.0, fmin(160.0, input_shared_fov));

    s3d_pos forward_vec = {0};
    forward_vec.x = 1000.0;
    s3d_rotation horizontal_half_fov = {0};
    horizontal_half_fov.hori = input_shared_fov / 2;
    spew3d_math3d_rotate(&forward_vec, &horizontal_half_fov);

    assert(forward_vec.y > 0 &&
        forward_vec.x < 1000.0 &&
        forward_vec.x > 0);

    double square_avg_plane_half = (
        ((double)pixel_width + (double)pixel_height) / 2.0
    ) / 2.0;
    double scaler = (
        (double)square_avg_plane_half / (double)forward_vec.y
    );

    forward_vec.x *= scaler;
    forward_vec.y = (double)pixel_width / 2.0;
    s3d_pos new_forward_vec = {0};
    new_forward_vec.x = spew3d_math3d_len(forward_vec);
    double hori_fov = spew3d_math3d_anglefromto(
        &new_forward_vec, &forward_vec
    ) * 2;
    *output_horifov = hori_fov;
    forward_vec.y = (double)pixel_height / 2.0;
    double verti_fov = spew3d_math3d_anglefromto(
        &new_forward_vec, &forward_vec
    ) * 2;
    *output_vertifov = verti_fov;
}

S3DEXP s3dnum_t spew3d_math3d_anglefromto(
        s3d_pos *p, s3d_pos *p2
        ) {
    s3dnum_t d = p->x*p2->x + p->y*p2->y + p->z*p2->z;
    s3dnum_t cx = (p->y*p2->z - p->z*p2->y);
    s3dnum_t cy = (p->z*p2->x - p->x*p2->z);
    s3dnum_t cz = (p->x*p2->y - p->y*p2->x);
    s3dnum_t de = sqrt(cx*cx + cy*cy + cz*cz);
    s3dnum_t angle = atan2(de, d);
    return (angle / M_PI) * 180.0;
}

S3DEXP void spew3d_math3d_transform3d(
        s3d_pos input_pos,
        s3d_transform3d_cam_info *cam_info,
        s3d_pos model_world_pos,
        s3d_rotation model_world_rotation,
        s3d_pos *out_pos
        ) {
    spew3d_math3d_rotate(&input_pos, &model_world_rotation);
    spew3d_math3d_add(&input_pos, &model_world_pos);
    spew3d_math3d_sub(&input_pos, &cam_info->cam_pos);

    s3d_rotation reverse;
    reverse.roll = -cam_info->cam_rotation.roll;
    reverse.hori = -cam_info->cam_rotation.hori;
    reverse.verti = -cam_info->cam_rotation.verti;

    spew3d_math3d_rotate(&input_pos, &reverse);
    if (S3DUNLIKELY(!cam_info->cache_set)) {
        cam_info->cache_set = 1;
        cam_info->cached_screen_plane_x = 10000;

        s3d_point vec2d_to_left_side;
        vec2d_to_left_side.x =
            cam_info->cached_screen_plane_x;
        spew3d_math2d_rotate(&vec2d_to_left_side,
            cam_info->cam_horifov);
        assert(vec2d_to_left_side.x > 0);
        spew3d_math2d_scale(&vec2d_to_left_side,
            cam_info->cached_screen_plane_x /
            vec2d_to_left_side.x);
        assert(vec2d_to_left_side.y > 0);
        cam_info->cached_screen_plane_yleftoffset = (
            -vec2d_to_left_side.y
        );

        s3d_point vec2d_to_top_end;
        vec2d_to_top_end.x =
            cam_info->cached_screen_plane_x;
        spew3d_math2d_rotate(&vec2d_to_top_end,
            cam_info->cam_vertifov);
        assert(vec2d_to_top_end.x > 0);
        spew3d_math2d_scale(&vec2d_to_top_end,
            cam_info->cached_screen_plane_x /
            vec2d_to_top_end.x);
        assert(vec2d_to_top_end.y > 0);
        cam_info->cached_screen_plane_ztopoffset = (
            vec2d_to_top_end.y
        );

        cam_info->cached_screen_plane_pixel_ywidth = (
            (double)cam_info->cached_screen_plane_yleftoffset /
            ((double)cam_info->viewport_pixel_width / 2.0)
        );
        cam_info->cached_screen_plane_pixel_zwidth = (
            (double)cam_info->cached_screen_plane_yleftoffset /
            ((double)cam_info->viewport_pixel_width / 2.0)
        );
        cam_info->cached_screen_plane_pixel_mixedwidth = (
            (double)cam_info->cached_screen_plane_pixel_ywidth +
            (double)cam_info->cached_screen_plane_pixel_zwidth
        ) / 2.0;
    }

    /*if (enable_scaling_depth)
        input_pos.x = (
            input_pos.x /
            cam_info->cached_screen_plane_pixel_mixedwidth
        );*/
    input_pos.y = (
        input_pos.y /
        cam_info->cached_screen_plane_pixel_ywidth
    );
    input_pos.z = (
        input_pos.z /
        cam_info->cached_screen_plane_pixel_ywidth
    );
    *out_pos = input_pos;
}

S3DEXP void spew3d_math3d_rotate(
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
    double py = -p->y;
    double pz = p->z;

    // Roll angle:
    newy = py * cos(rotr) + pz * sin(rotr);
    newz = pz * cos(rotr) - py * sin(rotr);
    py = newy;
    pz = newz;

    // Vertical angle:
    newz = pz * cos(rotv) + px * sin(rotv);
    newx = px * cos(rotv) - pz * sin(rotv);
    px = newx;
    pz = newz;

    // Horizontal angle:
    newy = py * cos(roth) + px * sin(roth);
    newx = px * cos(roth) - py * sin(roth);
    px = newx;
    py = newy;

    p->x = newx;
    p->y = -newy;
    p->z = newz;
}

#endif  // SPEW3D_IMPLEMENTATION

