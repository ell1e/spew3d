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

#ifndef SPEW3D_SCENE3D_H_
#define SPEW3D_SCENE3D_H_

#include <stdint.h>

enum Obj3dKind {
    OBJ3D_INVALID = 0,
    OBJ3D_INVISIBLE = 1,
    OBJ3D_MESH,
    OBJ3D_SPRITE3D,
    OBJ3D_CAMERA,
    OBJ3D_LVLBOX
};

typedef struct s3d_scenecolorinfo {
    s3d_color ambient_emit;
} s3d_scenecolorinfo;

typedef struct s3d_spatialstore3d s3d_spatialstore3d;
typedef struct s3d_obj3d s3d_obj3d;
typedef struct s3d_scene3d s3d_scene3d;

S3DEXP s3d_scene3d *spew3d_scene3d_New(
    double max_coord_range, double max_regular_collision_size
);

S3DEXP s3d_scenecolorinfo spew3d_scene3d_GetColorInfo(
    s3d_scene3d *sc
);

S3DEXP s3d_obj3d *spew3d_scene3d_AddMeshObj(
    s3d_scene3d *sc, s3d_geometry *geom, int object_owns_mesh
);

S3DEXP s3d_obj3d *spew3d_scene3d_AddLvlboxObj(
    s3d_scene3d *sc, s3d_lvlbox *lvlbox,
    int object_owns_lvlbox
);

S3DEXP int spew3d_scene3d_AddPreexistingObj(
    s3d_scene3d *sc, s3d_obj3d *obj
);

S3DEXP s3d_pos spew3d_obj3d_GetPos(s3d_obj3d *obj);

S3DEXP s3d_rotation spew3d_obj3d_GetRotation(s3d_obj3d *obj);

S3DEXP s3d_spatialstore3d *spew3d_scene3d_GetStore(
    s3d_scene3d *sc
);

S3DEXP s3d_spatialstore3d *spew3d_scene3d_GetStoreByObj3d(
    s3d_obj3d *obj
);

S3DEXP void spew3d_obj3d_LockAccess(s3d_obj3d *obj);

S3DEXP void spew3d_obj3d_ReleaseAccess(s3d_obj3d *obj);

S3DEXP void spew3d_obj3d_SetPos(
    s3d_obj3d *obj, s3d_pos pos
);

S3DEXP void spew3d_obj3d_SetRotation(
    s3d_obj3d *obj, s3d_rotation rot
);

S3DEXP void spew3d_scene3d_Destroy(s3d_scene3d *sc);

S3DEXP double spew3d_obj3d_GetOuterMaxExtentRadius(s3d_obj3d *obj);

S3DEXP int spew3d_obj3d_AddCustomTypeNum(s3d_obj3d *obj, int32_t typeno);

S3DEXP int spew3d_obj3d_HasCustomTypeNum(s3d_obj3d *obj, int32_t typeno);

S3DEXP void spew3d_obj3d_Destroy(s3d_obj3d *obj);

#endif  // SPEW3D_SCENE3D_H_

