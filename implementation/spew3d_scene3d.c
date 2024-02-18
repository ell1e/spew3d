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

/** XXX: NOTE: IMPORTANT INFO ABOUT MULTI-THREADING:
 *  ================================================
 *
 * Spew3D's threading makes the following assumptions about objects:
 *
 * - If the scene isn't locked, position and rotation of any object
 *   can change around at any time and might be corrupt on access.
 *
 * - However, an object's type never changes. Also, its mesh never
 *   changes as long as it's in the scene. (The rendering will corrupt
 *   if these assumptions are violated.)
 */

#ifdef SPEW3D_IMPLEMENTATION

typedef struct s3d_spatialstore3d s3d_spatialstore3d;

typedef struct s3d_scene3d {
    s3d_mutex *m;
    s3d_spatialstore3d *store;
} s3d_scene3d;

typedef struct s3d_obj3d {
    int kind;
    int wasdeleted;
    s3d_scene3d *owner;
    s3d_pos pos;
    s3d_rotation rot;
    int32_t custom_type_nums[8];
    void *extra;
    void (*extra_destroy_cb)(s3d_obj3d *obj, void *extra);
} s3d_obj3d;

S3DHID int _spew3d_scene3d_GetKind_nolock(s3d_obj3d *obj) {
    return obj->kind;
}

S3DHID void _spew3d_scene3d_SetKind_nolock(
        s3d_obj3d *obj, int kind
        ) {
    obj->kind = kind;
}

S3DHID void *_spew3d_scene3d_ObjExtraData_nolock(s3d_obj3d *obj) {
    return obj->extra;
}

S3DHID void *spew3d_scene3d_ObjExtraData(s3d_obj3d *obj) {
    if (obj->owner) {
        mutex_Lock(obj->owner->m);
    }
    void *result = _spew3d_scene3d_ObjExtraData_nolock(obj);
    if (obj->owner) {
        mutex_Release(obj->owner->m);
    }
    return result;
}

S3DHID void _spew3d_scene3d_ObjSetExtraData_nolock(
        s3d_obj3d *obj, void *extra,
        void (*extra_destroy_cb)(s3d_obj3d *obj, void *extra)
        ) {
    obj->extra = extra;
}

S3DEXP s3d_scene3d *spew3d_scene3d_New(
        double max_coord_range, double max_regular_collision_size
        ) {
    s3d_scene3d *sc = malloc(sizeof(*sc));
    if (!sc)
        return NULL;
    memset(sc, 0, sizeof(*sc));
    sc->m = mutex_Create();
    if (!sc->m) {
        spew3d_scene3d_Destroy(sc);
        return NULL;
    }
    s3d_pos center = {0};
    sc->store = s3d_spatial3d_NewDefault(
        max_coord_range, max_regular_collision_size,
        center
    );
    if (!sc->store) {
        spew3d_scene3d_Destroy(sc);
        return NULL;
    }
    return sc;
}

S3DEXP s3d_spatialstore3d *spew3d_scene3d_GetStore(
        s3d_scene3d *sc
        ) {
    mutex_Lock(sc->m);
    s3d_spatialstore3d *result = sc->store;
    mutex_Release(sc->m);
    return result;
}

S3DEXP s3d_spatialstore3d *spew3d_scene3d_GetStoreByObj3d(
        s3d_obj3d *obj
        ) {
    if (obj->owner) {
        mutex_Lock(obj->owner->m);
        s3d_spatialstore3d *result = obj->owner->store;
        mutex_Release(obj->owner->m);
        return result;
    }
    return NULL;
}

S3DEXP int _spew3d_obj3d_GetWasDeleted_nolock(
        s3d_obj3d *obj
        ) {
    return obj->wasdeleted;
}

S3DEXP double spew3d_obj3d_GetOuterMaxExtentRadius_nolock(
        s3d_obj3d *obj) {
    return 0;
}

S3DEXP void _spew3d_obj3d_Lock(s3d_obj3d *obj) {
    if (obj->owner) {
        mutex_Lock(obj->owner->m);
    }
}

S3DEXP void _spew3d_obj3d_Unlock(s3d_obj3d *obj) {
    if (obj->owner) {
        mutex_Release(obj->owner->m);
    }
}

S3DEXP double spew3d_obj3d_GetOuterMaxExtentRadius(s3d_obj3d *obj) {
    if (obj->owner) {
        mutex_Lock(obj->owner->m);
    }
    double result = spew3d_obj3d_GetOuterMaxExtentRadius_nolock(
        obj
    );
    if (obj->owner) {
        mutex_Release(obj->owner->m);
    }
    return result;
}

S3DEXP int spew3d_scene3d_AddPreexistingObj(
        s3d_scene3d *sc, s3d_obj3d *obj
        ) {
    assert(sc != NULL);
    assert(sc->m != NULL);
    assert(sc->store != NULL);
    mutex_Lock(sc->m);
    obj->owner = sc;
    s3d_pos pos = obj->pos;
    double radius = spew3d_obj3d_GetOuterMaxExtentRadius_nolock(
        obj);
    int result = sc->store->Add(sc->store, obj,
        pos, radius, 0);
    mutex_Release(sc->m);
    return result;
}

S3DEXP void spew3d_scene3d_Destroy(s3d_scene3d *sc) {
    if (sc == NULL)
        return;

    if (sc->store != NULL)
        sc->store->Destroy(sc->store);
    if (sc->m)
        mutex_Destroy(sc->m);
    free(sc);
}

S3DEXP void spew3d_obj3d_Destroy(s3d_obj3d *obj) {
    if (!obj)
        return;
    s3d_scene3d *s = NULL;
    if (obj->owner) {
        s = obj->owner;
        mutex_Lock(s->m);
    }
    obj->wasdeleted = 1;
    if (s) {
        mutex_Release(s->m);
    }
}

S3DEXP void _spew3d_obj3d_DestroyActually(s3d_obj3d *obj) {
    if (!obj)
        return;
    assert(obj->wasdeleted);
    s3d_scene3d *s = NULL;
    if (obj->owner) {
        s = obj->owner;
        mutex_Lock(s->m);
        s->store->Remove(s->store, obj);
    }
    if (obj->extra) {
        if (obj->extra_destroy_cb) {
            obj->extra_destroy_cb(obj, obj->extra);
        } else {
            free(obj->extra);
        }
    }
    free(obj);
    if (s) {
        mutex_Release(s->m);
    }
}

S3DHID size_t spew3d_obj3d_GetStructSize() {
    return sizeof(s3d_obj3d);
}

S3DHID s3d_pos spew3d_obj3d_GetPos_nolock(s3d_obj3d *obj) {
    return obj->pos;
}

S3DEXP void spew3d_obj3d_LockAccess(s3d_obj3d *obj) {
    if (obj->owner) {
        mutex_Lock(obj->owner->m);
    }
}

S3DEXP void spew3d_obj3d_ReleaseAccess(s3d_obj3d *obj) {
    if (obj->owner) {
        mutex_Lock(obj->owner->m);
    }
}

S3DEXP s3d_pos spew3d_obj3d_GetPos(s3d_obj3d *obj) {
    if (obj->owner) {
        mutex_Lock(obj->owner->m);
    }
    s3d_pos pos = spew3d_obj3d_GetPos_nolock(obj);
    if (obj->owner) {
        mutex_Release(obj->owner->m);
    }
    return pos;
}

S3DHID void spew3d_obj3d_SetPos_nolock(
        s3d_obj3d *obj, s3d_pos pos) {
    obj->pos = pos;
}

S3DEXP void spew3d_obj3d_SetPos(
        s3d_obj3d *obj, s3d_pos pos
        ) {
    if (obj->owner) {
        mutex_Lock(obj->owner->m);
    }
    spew3d_obj3d_SetPos_nolock(obj, pos);
    if (obj->owner) {
        mutex_Release(obj->owner->m);
    }
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

