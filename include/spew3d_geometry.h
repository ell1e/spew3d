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

#ifndef SPEW3D_GEOMETRY_H_
#define SPEW3D_GEOMETRY_H_

#include <stdint.h>

typedef uint64_t s3d_texture_t;
typedef struct s3d_pos s3d_pos;
typedef struct s3d_point s3d_point;
typedef struct s3d_rotation s3d_rotation;

typedef uint32_t s3d_material_t;

#define SPEW3D_MATERIAL_TRANSPARENT33 ((uint32_t)1)
#define SPEW3D_MATERIAL_TRANSPARENT66 ((uint32_t)1 << 1)
#define SPEW3D_MATERIAL_OBJECTPASSABLE ((uint32_t)1 << 2)
#define SPEW3D_MATERIAL_BULLETSHOTPASSABLE ((uint32_t)1 << 3)
#define SPEW3D_MATERIAL_AISIGHTPASSABLE ((uint32_t)1 << 4)

typedef struct s3d_geometry {
    int wasdeleted;

    int32_t vertex_count;
    s3d_pos *vertex;
    int32_t polygon_count;
    s3d_pos *polygon_normal;
    s3d_material_t *polygon_material;
    uint32_t *polygon_vertexindex;
    s3d_point *polygon_texcoord;
    s3d_texture_t *polygon_texture;

    int per_polygon_emit_computed;
    s3d_color *polygon_vertexcolors;

    int per_vertex_normals_computed;
    s3d_pos *polygon_vertexnormals;

    int32_t owned_texture_count;
    s3d_texture_t *owned_texture;
} s3d_geometry;

S3DEXP s3d_geometry *spew3d_geometry_Create();

S3DEXP int spew3d_geometry_AddCube(
    s3d_geometry *geometry,
    s3dnum_t edge_width,
    s3d_pos *offset,
    s3d_rotation *rotation,
    s3d_point *side_texcoord,
    s3d_texture_t *side_texture,
    int *side_texture_owned
);

S3DEXP int spew3d_geometry_AddCubeSimple(
    s3d_geometry *geometry,
    s3dnum_t edge_width,
    s3d_texture_t texture,
    int texture_owned
);

S3DEXP int spew3d_geometry_AddPlane(
    s3d_geometry *geometry,
    s3dnum_t plane_width, s3dnum_t plane_height,
    s3d_pos *offset,
    s3d_rotation *rotation,
    s3d_point *side_texcoord,
    s3d_texture_t texture,
    int texture_owned
);

S3DEXP int spew3d_geometry_AddPlaneSimple(
    s3d_geometry *geometry,
    s3dnum_t plane_width, s3dnum_t plane_height,
    int faces_upward, s3d_texture_t texture,
    int texture_owned
);

S3DEXP void spew3d_geometry_Destroy(s3d_geometry *geometry);

enum GeomDynLightRenderDetail {
    DLRD_INVALID = 0,
    DLRD_UNLIT = 1,
    DLRD_LIT_FLAT = 2,
    DLRD_LIT_FULLY = 3,
};

typedef struct s3d_geometryrenderlightinfo {
    s3dnum_t alpha;
    int withalphachannel;
    int dynlight_mode;  // enum GeomDynLightRenderDetail.
    s3d_color ambient_emit;

    s3d_pos primary_light_pos;
    s3dnum_t primary_light_range;
    s3d_color primary_light_color;

    s3d_pos secondary_light_pos;
    s3dnum_t secondary_light_range;
    s3d_color secondary_light_color;
} s3d_geometryrenderlightinfo;

S3DEXP int spew3d_geometry_Transform(
    s3d_geometry *geometry,
    s3d_pos *model_pos,
    s3d_rotation *model_rotation,
    s3d_transform3d_cam_info *cam_info,
    s3d_geometryrenderlightinfo *render_light_info,
    s3d_renderpolygon **render_queue,
    uint32_t *render_fill, uint32_t *render_alloc
);

#endif  // SPEW3D_GEOMETRY_H_

