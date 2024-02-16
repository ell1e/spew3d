/* Copyright (c) 2024, ellie/@ell1e & Spew3D Team (see AUTHORS.md).

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

#ifndef SPEW3D_SPATIALSTORE3D_H_
#define SPEW3D_SPATIALSTORE3D_H_

typedef struct s3d_obj3d s3d_obj3d;
typedef struct s3d_spatialstore3d s3d_spatialstore3d;
typedef struct s3d_pos s3d_pos;

typedef struct s3d_spatialstore3d {
    int (*Add)(s3d_spatialstore3d *store, s3d_obj3d *obj, 
        s3d_pos pos, double extent_outer_radius, int is_static);
    int (*Remove)(s3d_spatialstore3d *store, s3d_obj3d *obj);
    int (*Find)(s3d_spatialstore3d *store, s3d_pos searchpos,
        double searchrange, int expand_scan_by_collision_size,
        s3d_obj3d **out_list, uint32_t *out_count);
    int (*FindByCustomTypeNo)(s3d_spatialstore3d *store, s3d_pos searchpos,
        double searchrange, int expand_scan_by_collision_size,
        int32_t *custom_type_no_list, uint32_t custom_type_no_list_len,
        s3d_obj3d **out_list, uint32_t *out_count);
    int (*FindEx)(s3d_spatialstore3d *store, s3d_pos searchpos,
        double searchrange, int expand_scan_by_collision_size,
        int32_t *custom_type_no_list, uint32_t custom_type_no_list_len,
        s3d_obj3d **buffer_for_list,
        int buffer_alloc, s3d_obj3d **out_list,
        uint32_t *out_count, uint32_t *out_buffer_alloc);
    int (*FindClosest)(s3d_spatialstore3d *store, s3d_pos searchpos,
        double searchrange, int expand_scan_by_collision_size,
        s3d_obj3d *out_obj);
    void *internal_data;
} s3d_spatialstore3d;

S3DEXP s3d_spatialstore3d *s3d_spatial3d_NewDefault(
    double max_coord_range, double max_regular_collision_size,
    s3d_pos center
);

#endif  // SPEW3D_SPATIALSTORE3D_H_

