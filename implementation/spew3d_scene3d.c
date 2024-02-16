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

#ifdef SPEW3D_IMPLEMENTATION

typedef struct s3d_scene3d {
    
} s3d_scene3d;

typedef struct s3d_obj3d {
    s3d_pos pos;
    int32_t custom_type_nums[8];
} s3d_obj3d;

S3DEXP s3d_scene3d *spew3d_scene3d_New(double max_coord_range) {
    return NULL;
}

S3DEXP s3d_pos spew3d_obj3d_GetPos(s3d_obj3d *obj) {
    return obj->pos;
}

S3DEXP int spew3d_obj3d_AddCustomTypeNum(
        s3d_obj3d *obj, int32_t typeno
        ) {
    if (typeno < 0)
        return 0;
    int k = 0;
    while (k < 8) {
        if (obj->custom_type_nums[k] < 0) {
            obj->custom_type_nums[k] = typeno;
            return 1;
        }
        k++;
    }
    return 1;
}

S3DEXP int spew3d_obj3d_HasCustomTypeNum(
        s3d_obj3d *obj, int32_t typeno
        ) {
    if (typeno < 0)
        return 0;
    int k = 0;
    while (k < 8) {
        if (obj->custom_type_nums[k] == typeno) {
            return 1;
        }
        k++;
    }
    return 0;
}

#endif  // SPEW3D_IMPLEMENTATION
