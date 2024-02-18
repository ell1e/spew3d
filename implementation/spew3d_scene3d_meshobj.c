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

extern s3d_mutex *_win_id_mutex;
typedef struct s3d_window s3d_window;

S3DHID size_t spew3d_obj3d_GetStructSize();
S3DEXP void _spew3d_obj3d_Lock(s3d_obj3d *obj);
S3DEXP void _spew3d_obj3d_Unlock(s3d_obj3d *obj);
S3DHID void *_spew3d_scene3d_ObjExtraData_nolock(s3d_obj3d *obj);
S3DHID void _spew3d_scene3d_ObjSetExtraData_nolock(
    s3d_obj3d *obj, void *data,
    void (*extra_destroy_cb)(s3d_obj3d *obj, void *extra)
);
S3DHID int _spew3d_scene3d_GetKind_nolock(s3d_obj3d *obj);
S3DHID void _spew3d_scene3d_SetKind_nolock();
S3DHID s3d_window *_spew3d_window_GetByIDLocked(uint32_t id);

typedef struct spew3d_meshobjdata {
    int owning_mesh;
    s3d_geometry *geom;
} spew3d_meshobjdata;

S3DHID void spew3d_scene3d_MeshObjFreeData(
        s3d_obj3d *obj, void *extra
        ) {
    spew3d_meshobjdata *mdata = (
        (spew3d_meshobjdata *)extra
    );
    if (mdata->owning_mesh && mdata->geom != NULL) {
        spew3d_geometry_Destroy(mdata->geom);
    }
    free(mdata);
}

S3DEXP s3d_obj3d *spew3d_scene3d_AddMeshObj(
        s3d_scene3d *sc, s3d_geometry *geom,
        int object_owns_mesh
        ) {
    assert(sc != NULL);
    s3d_obj3d *obj = malloc(spew3d_obj3d_GetStructSize());
    if (!obj)
        return NULL;
    memset(obj, 0, spew3d_obj3d_GetStructSize());
    _spew3d_scene3d_SetKind_nolock(obj, OBJ3D_MESH);

    spew3d_meshobjdata *objdata = malloc(sizeof(*objdata));
    if (!objdata) {
        spew3d_obj3d_Destroy(obj);
        return NULL;
    }
    memset(objdata, 0, sizeof(*objdata));
    objdata->owning_mesh = object_owns_mesh;
    objdata->geom = geom;
    _spew3d_scene3d_ObjSetExtraData_nolock(
        obj, objdata, spew3d_scene3d_MeshObjFreeData);

    int result = spew3d_scene3d_AddPreexistingObj(
        sc, obj
    );
    if (!result) {
        spew3d_obj3d_Destroy(obj);
        return NULL;
    }
    return obj;
}

#endif  // SPEW3D_IMPLEMENTATION

