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

#if defined(SPEW3D_IMPLEMENTATION) && \
    SPEW3D_IMPLEMENTATION != 0

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
    new_forward_vec.x = forward_vec.x;
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

S3DEXP void spew3d_math3d_cross_product(
        s3d_pos *v1, s3d_pos *v2, s3d_pos *out
        ) {
    s3dnum_t x = ((v1->y * v2->z) - (v1->z * v2->y));
    s3dnum_t y = ((v1->z * v2->x) - (v1->x * v2->z));
    s3dnum_t z = ((v1->x * v2->y) - (v1->y * v2->x));
    out->x = x;
    out->y = y;
    out->z = z;
}

S3DEXP void spew3d_math3d_sample_polygon_texcoord(
        s3d_transform3d_cam_info *cam_info,
        s3d_pos *v1, s3d_pos *v2, s3d_pos *v3,
        s3d_point *v1tx, s3d_point *v2tx, s3d_point *v3tx,
        s3d_pos sample_at,
        s3d_pos *out_sample_at_with_depth,
        s3d_pos *out_sample_pixel_pos,
        s3d_point *out_sampletx
        ) {
    // FIXME: This is a horribly slow way to go about it,
    // and not how this should be done properly.
    // However, for just the occasional sampling rather than
    // per pixel, this will do the job for now.

    s3d_pos v1frontal = *v1;
    s3d_pos v2frontal = *v2;
    s3d_pos v3frontal = *v3;
    s3d_pos center = v1frontal;
    s3d_pos samplefrontal = sample_at;
    s3d_pos normal;
    spew3d_math3d_polygon_normal(
        v1, v2, v3, 0, &normal
    );
    if (normal.x < 0 && 0) {
        spew3d_math3d_flip(&normal);
    }
    s3d_rotation normalrot = spew3d_math3d_rotationfromto(
        NULL, &normal
    );
    s3d_rotation reverse1 = {0};
    s3d_rotation reverse2 = {0};
    reverse1.hori = -normalrot.hori;
    reverse2.verti = -normalrot.verti;
    spew3d_math3d_rotateat(&v1frontal, &reverse1, &center);
    spew3d_math3d_rotateat(&v1frontal, &reverse2, &center);
    spew3d_math3d_rotateat(&v2frontal, &reverse1, &center);
    spew3d_math3d_rotateat(&v2frontal, &reverse2, &center);
    spew3d_math3d_rotateat(&v3frontal, &reverse1, &center);
    spew3d_math3d_rotateat(&v3frontal, &reverse2, &center);
    spew3d_math3d_rotateat(&samplefrontal, &reverse1, &center);
    spew3d_math3d_rotateat(&samplefrontal, &reverse2, &center);
    samplefrontal.x = v1frontal.x;
    v2frontal.x = v1frontal.x;
    v3frontal.x = v1frontal.x;
    s3dnum_t sampletov1 = spew3d_math3d_dist(
        &samplefrontal, &v1frontal
    );
    s3dnum_t sampletov2 = spew3d_math3d_dist(
        &samplefrontal, &v2frontal
    );
    s3dnum_t sampletov3 = spew3d_math3d_dist(
        &samplefrontal, &v3frontal
    );
    s3dnum_t v1tov2 = spew3d_math3d_dist(
        v1, v2
    );
    s3dnum_t v2tov3 = spew3d_math3d_dist(
        v2, v3
    );
    s3dnum_t v3tov1 = spew3d_math3d_dist(
        v3, v1
    );
    if (out_sample_at_with_depth != NULL ||
            out_sample_pixel_pos != NULL
            ) {
        sample_at = samplefrontal;
        spew3d_math3d_rotateat(&sample_at, &normalrot, &center);
        if (out_sample_at_with_depth != NULL)
            *out_sample_at_with_depth = sample_at;
        if (out_sample_pixel_pos != NULL) {
            spew3d_math3d_transform3dscreenspace(
                sample_at, cam_info,
                &sample_at, NULL
            );
            *out_sample_pixel_pos = sample_at;
        }
    }
    if (out_sampletx != NULL) {
        s3dnum_t sum = fmax(1, sampletov1 + sampletov2 + sampletov3);

        if (sampletov3 < 0.01) {
            out_sampletx->x = v3tx->x;
            out_sampletx->y = v3tx->y;
            return;
        }
        if (sampletov2 < 0.01) {
            out_sampletx->x = v2tx->x;
            out_sampletx->y = v2tx->y;
            return;
        }
        if (sampletov1 < 0.01) {
            out_sampletx->x = v1tx->x;
            out_sampletx->y = v1tx->y;
            return;
        }
        if ((sampletov1 + sampletov3) - v3tov1 < 0.01) {
            double f_x = sampletov3 / v3tov1;
            out_sampletx->x = (1 - f_x) * v3tx->x +
                f_x * v1tx->x;
            out_sampletx->y = (1 - f_x) * v3tx->y +
                f_x * v1tx->y;
            return;
        }
        if ((sampletov2 + sampletov3) - v2tov3 < 0.01) {
            double f_x = sampletov3 / v2tov3;
            out_sampletx->x = (1 - f_x) * v3tx->x +
                f_x * v2tx->x;
            out_sampletx->y = (1 - f_x) * v3tx->y +
                f_x * v2tx->y;
            return;
        }
        if ((sampletov1 + sampletov2) - v1tov2 < 0.01) {
            double f_x = sampletov1 / v1tov2;
            out_sampletx->x = (1 - f_x) * v1tx->x +
                f_x * v2tx->x;
            out_sampletx->y = (1 - f_x) * v1tx->y +
                f_x * v2tx->y;
            return;
        }

        s3dnum_t largest_dist = fmax(fmax(
            sampletov1, sampletov2
        ), sampletov3) * 3;
        s3d_point v3_to_sample_vec = {0};
        v3_to_sample_vec.x = (samplefrontal.y - v3frontal.y);
        v3_to_sample_vec.y = (samplefrontal.z - v3frontal.z);
        spew3d_math2d_scaletolen(&v3_to_sample_vec, largest_dist);
        s3d_point projected_on_v1tov2;
        s3d_point v1_2d;
        v1_2d.x = v1frontal.y;
        v1_2d.y = v1frontal.z;
        s3d_point v2_2d;
        v2_2d.x = v2frontal.y;
        v2_2d.y = v2frontal.z;
        s3d_point v3_2d;
        v3_2d.x = v3frontal.y;
        v3_2d.y = v3frontal.z;
        s3d_point ray_target = v3_2d;
        ray_target.x += v3_to_sample_vec.x;
        ray_target.y += v3_to_sample_vec.y;
        if (!spew3d_math2d_lineintersect(
                &v1_2d, &v2_2d,
                &v3_2d, &ray_target,
                &projected_on_v1tov2
                )) {
            printf("INTERSECT NO??\n");
            if (sampletov1 < sampletov2 &&
                    sampletov1 < sampletov3) {
                out_sampletx->x = v1tx->x;
                out_sampletx->y = v1tx->y;
            } else if (sampletov2 < sampletov1 &&
                    sampletov2 < sampletov3) {
                out_sampletx->x = v2tx->x;
                out_sampletx->y = v2tx->y;
            } else {
                out_sampletx->x = v3tx->x;
                out_sampletx->y = v3tx->y;
            }
        } else {
            /*printf("v1.x %f v1.y %f v2.x %f v2.y %f "
                "v3.x %f v3.y %f target.x %f target.y %f "
                "sample.x %f sample.y %f\n",
                (double)v1_2d.x, (double)v1_2d.y,
                (double)v2_2d.x, (double)v2_2d.y,
                (double)v3_2d.x, (doule)v3_2d.y,
                (double)ray_target.x, (double)ray_target.y,
                (double)samplefrontal.y, (double)samplefrontal.z);
            printf("intersect yes intersect.x %f intersect.y %f\n",
                (double)projected_on_v1tov2.x,
                (double)projected_on_v1tov2.y);*/
            s3dnum_t tx_f = fmax(0, fmin(1, spew3d_math2d_dist(
                &v1_2d, &projected_on_v1tov2
            ) / v1tov2));
            s3dnum_t tx = (1 - tx_f) * v1tx->x +
                tx_f * v2tx->x;
            s3dnum_t ty = (1 - tx_f) * v1tx->y +
                tx_f * v2tx->y;
            s3d_point samplefrontal2d;
            samplefrontal2d.x = samplefrontal.y;
            samplefrontal2d.y = samplefrontal.z;
            s3dnum_t full_dist_to_v3 = spew3d_math2d_dist(
                &v3_2d, &projected_on_v1tov2
            );
            tx_f = fmax(0, fmin(1, spew3d_math2d_dist(
                &v3_2d, &samplefrontal2d
            ) / full_dist_to_v3));
            tx = (1 - tx_f) * v3tx->x +
                tx_f * tx;
            ty = (1 - tx_f) * v3tx->y +
                tx_f * ty;
            out_sampletx->x = tx;
            out_sampletx->y = ty;
        }
    }
}

S3DEXP void spew3d_math3d_polygon_normal(
        s3d_pos *v1, s3d_pos *v2, s3d_pos *v3,
        int do_normalize,
        s3d_pos *out
        ) {
    s3d_pos normal;
    s3d_pos input1;
    input1.x = v2->x - v1->x;
    input1.y = v2->y - v1->y;
    input1.z = v2->z - v1->z;
    s3d_pos input2;
    input2.x = v3->x - v1->x;
    input2.y = v3->y - v1->y;
    input2.z = v3->z - v1->z;
    spew3d_math3d_cross_product(&input1, &input2,
        &normal);
    if (do_normalize) {
        s3dnum_t len = spew3d_math3d_len(normal);
        if (len >= (s3dnum_t)0.00001) {
            normal.x /= len;
            normal.y /= len;
            normal.z /= len;
        } else {
            normal.x = 0;
            normal.y = 0;
            normal.z = 0;
        }
    }
    *out = normal;
}

S3DEXP s3dnum_t spew3d_math3d_anglefromto(
        s3d_pos *p, s3d_pos *p2
        ) {
    s3d_pos nullpos;
    if (p == NULL)
        p = &nullpos;
    s3dnum_t d = p->x*p2->x + p->y*p2->y + p->z*p2->z;
    s3dnum_t cx = (p->y*p2->z - p->z*p2->y);
    s3dnum_t cy = (p->z*p2->x - p->x*p2->z);
    s3dnum_t cz = (p->x*p2->y - p->y*p2->x);
    s3dnum_t de = sqrt(cx*cx + cy*cy + cz*cz);
    s3dnum_t angle = atan2(de, d);
    return (angle / M_PI) * 180.0;
}

S3DEXP s3d_rotation spew3d_math3d_rotationfromto(
        s3d_pos *p, s3d_pos *p2
        ) {
    s3d_pos nullpos;
    if (p == NULL) {
        memset(&nullpos, 0, sizeof(nullpos));
        p = &nullpos;
    }

    s3d_rotation rot = {0};

    s3d_pos horidiff = {0};
    horidiff.x = p2->x - p->x;
    horidiff.y = p2->y - p->y;
    if (spew3d_math3d_len(horidiff) > 0.001) {
        s3d_pos horiforward = {0};
        horiforward.x = spew3d_math3d_len(horidiff);
        rot.hori = spew3d_math3d_anglefromto(
            &horiforward, &horidiff
        );
        if (horidiff.y < 0)
            rot.hori = -rot.hori;
    }

    s3d_pos vertidiff = {0};
    vertidiff.z = p2->z - p->z;
    vertidiff.x = spew3d_math3d_len(horidiff);
    if (spew3d_math3d_len(vertidiff) > 0.001) {
        s3d_pos vertiforward = {0};
        vertiforward.x = spew3d_math3d_len(vertidiff);
        rot.verti = spew3d_math3d_anglefromto(
            &vertiforward, &vertidiff
        );
        if (vertidiff.z < 0)
            rot.verti = -rot.verti;
    }

    return rot;
}

S3DEXP void spew3d_math3d_transform3d(
        s3d_pos input_pos,
        s3d_transform3d_cam_info *cam_info,
        s3d_pos model_world_pos,
        s3d_rotation model_world_rotation,
        s3d_pos *out_pos, s3d_pos *out_unscaled_pos
        ) {
    /*printf("INPUT POS BEFORE MODEL TRANSFORM: %f,%f,%f\n",
        (double)input_pos.x, (double)input_pos.y,
        (double)input_pos.z);
    printf("MODEL WORLD POS %f, %f, %f\n",
        (double)model_world_pos.x,
        (double)model_world_pos.y,
        (double)model_world_pos.z);*/
    spew3d_math3d_rotate(&input_pos, &model_world_rotation);
    spew3d_math3d_add(&input_pos, &model_world_pos);
    /*printf("INPUT POS AFTER MODEL TRANSFORM: %f,%f,%f\n",
        (double)input_pos.x, (double)input_pos.y,
        (double)input_pos.z);
    printf("CAM WORLD POS %f, %f, %f, "
        "ANGLES roll=%f,hori=%f,verti=%f\n",
        (double)cam_info->cam_pos.x,
        (double)cam_info->cam_pos.y,
        (double)cam_info->cam_pos.z,
        (double)cam_info->cam_rotation.roll,
        (double)cam_info->cam_rotation.hori,
        (double)cam_info->cam_rotation.verti);*/
    spew3d_math3d_sub(&input_pos, &cam_info->cam_pos);

    s3d_rotation reverse2 = {0};
    reverse2.hori = -cam_info->cam_rotation.hori;
    spew3d_math3d_rotate(&input_pos, &reverse2);
    s3d_rotation reverse1 = {0};
    reverse1.verti = -cam_info->cam_rotation.verti;
    spew3d_math3d_rotate(&input_pos, &reverse1);
    s3d_rotation reverse3 = {0};
    reverse3.roll = -cam_info->cam_rotation.roll;
    spew3d_math3d_rotate(&input_pos, &reverse3);

    spew3d_math3d_transform3dscreenspace(
        input_pos, cam_info, out_pos, out_unscaled_pos
    );
}

S3DEXP void spew3d_math3d_transform3dscreenspace(
        s3d_pos input_pos,
        s3d_transform3d_cam_info *cam_info,
        s3d_pos *out_pos, s3d_pos *out_unscaled_pos
        ) {
    /*printf("INPUT POS AFTER CAM SPACE TRANSFORM: %f,%f,%f\n",
        (double)input_pos.x, (double)input_pos.y,
        (double)input_pos.z);*/
    if (S3DUNLIKELY(!cam_info->cache_set)) {
        cam_info->cache_set = 1;
        cam_info->cached_screen_plane_x = 10000;

        s3d_point vec2d_to_left_side = {0};
        vec2d_to_left_side.y = 0;
        vec2d_to_left_side.x =
            cam_info->cached_screen_plane_x;
        spew3d_math2d_rotate(&vec2d_to_left_side,
            cam_info->cam_horifov / 2.0);
        assert(vec2d_to_left_side.x > 0);
        spew3d_math2d_scale(&vec2d_to_left_side,
            cam_info->cached_screen_plane_x /
            vec2d_to_left_side.x);
        assert(vec2d_to_left_side.y > 0);
        cam_info->cached_screen_plane_yleftoffset = (
            -vec2d_to_left_side.y
        );

        s3d_point vec2d_to_top_end = {0};
        vec2d_to_top_end.y = 0;
        vec2d_to_top_end.x =
            cam_info->cached_screen_plane_x;
        spew3d_math2d_rotate(&vec2d_to_top_end,
            cam_info->cam_vertifov / 2.0);
        assert(vec2d_to_top_end.x > 0);
        spew3d_math2d_scale(&vec2d_to_top_end,
            cam_info->cached_screen_plane_x /
            vec2d_to_top_end.x);
        assert(vec2d_to_top_end.y > 0);
        cam_info->cached_screen_plane_ztopoffset = (
            vec2d_to_top_end.y
        );

        cam_info->cached_screen_plane_pixel_ywidth = (
            fabs((double)cam_info->cached_screen_plane_yleftoffset) /
            ((double)cam_info->viewport_pixel_width / 2.0)
        );
        cam_info->cached_screen_plane_pixel_zwidth = (
            (double)cam_info->cached_screen_plane_ztopoffset /
            ((double)cam_info->viewport_pixel_height / 2.0)
        );
        cam_info->cached_screen_plane_pixel_mixedwidth = (
            (double)cam_info->cached_screen_plane_pixel_ywidth +
            (double)cam_info->cached_screen_plane_pixel_zwidth
        ) / 2.0;
    }

    double dist_to_plane_factor = fmin(
        fabs(input_pos.x / cam_info->cached_screen_plane_x),
        9999
    );
    /*printf("RENDER PLANE DIMENSIONS: yleftoffset=%f,"
        "ztopoffset=%f, depth=%f, render plane aspect=%f, "
        "canvasw,h=%f,%f hori fov=%f verti fov=%f "
        "canvas aspect=%f "
        "dist_to_plane_factor=%f "
        "pixel slice ywidth=%f, pixel slice zwidth=%f\n",
        (double)cam_info->cached_screen_plane_yleftoffset,
        (double)cam_info->cached_screen_plane_ztopoffset,
        (double)cam_info->cached_screen_plane_x,
        (double)cam_info->cached_screen_plane_yleftoffset /
        (double)cam_info->cached_screen_plane_ztopoffset,
        (double)cam_info->viewport_pixel_width,
        (double)cam_info->viewport_pixel_height,
        (double)cam_info->cam_horifov,
        (double)cam_info->cam_vertifov,
        (double)cam_info->viewport_pixel_width /
        (double)cam_info->viewport_pixel_height,
        (double)dist_to_plane_factor,
        (double)cam_info->cached_screen_plane_pixel_ywidth,
        (double)cam_info->cached_screen_plane_pixel_zwidth
    );*/
    if (out_pos != NULL) {
        out_pos->y = (
            input_pos.y / (
            cam_info->cached_screen_plane_pixel_ywidth *
            dist_to_plane_factor)
        ) + (double)cam_info->viewport_pixel_width / 2.0;
        out_pos->z = (double)cam_info->viewport_pixel_height - ((
            input_pos.z / (
            cam_info->cached_screen_plane_pixel_zwidth *
            dist_to_plane_factor)
        ) + ((double)cam_info->viewport_pixel_height / 2.0));
        out_pos->x = input_pos.x;
    }
    if (out_unscaled_pos != NULL)
        *out_unscaled_pos = input_pos;
}

S3DEXP void spew3d_math3d_rotate(
        s3d_pos *p, s3d_rotation *r
        ) {
    /// Rotate a given pos around its origin by the given degrees.
    /// Positive angle gives CW (clockwise) rotation.
    /// X is forward (into screen), Y is right, Z is up.

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
    p->y = newy;
    p->z = newz;
}

#endif  // SPEW3D_IMPLEMENTATION

