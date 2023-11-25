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

#ifndef SPEW3D_GEOMETRY_H_
#define SPEW3D_GEOMETRY_H_

#include <stdint.h>

typedef uint64_t spew3d_texture_t;
typedef struct spew3d_pos spew3d_pos;
typedef struct spew3d_point spew3d_point;
typedef struct spew3d_rotation spew3d_rotation;

typedef uint32_t spew3d_material_t;

#define SPEW3D_MATERIAL_TRANSPARENT33 ((uint32_t)1)
#define SPEW3D_MATERIAL_TRANSPARENT66 ((uint32_t)1 << 1)
#define SPEW3D_MATERIAL_OBJECTPASSABLE ((uint32_t)1 << 2)
#define SPEW3D_MATERIAL_BULLETSHOTPASSABLE ((uint32_t)1 << 3)
#define SPEW3D_MATERIAL_AISIGHTPASSABLE ((uint32_t)1 << 4)


typedef struct spew3d_geometry {
    int32_t vertex_count;
    spew3d_pos *vertex;
    int32_t polygon_count;
    spew3d_pos *polygon_normal;
    spew3d_material_t *polygon_material;
    int32_t *polygon_vertexindex;
    spew3d_point *polygon_texcoord;
    spew3d_texture_t *polygon_texture;

    int32_t owned_texture_count;
    spew3d_texture_t *owned_texture;
} spew3d_geometry;


spew3d_geometry *spew3d_geometry_Create();


S3DEXP int spew3d_geometry_AddCube(
    spew3d_geometry *geometry,
    s3dnum_t edge_width,
    spew3d_pos *offset,
    spew3d_rotation *rotation,
    spew3d_point *side_texcoord,
    spew3d_texture_t *side_texture,
    int *side_texture_owned
);

int spew3d_geometry_AddCubeSimple(
    spew3d_geometry *geometry,
    s3dnum_t edge_width,
    spew3d_texture_t texture,
    int texture_owned
);

void spew3d_geometry_Destroy(spew3d_geometry *geometry);

#endif  // SPEW3D_GEOMETRY_H_

