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

#include <stdint.h>

extern s3d_mutex *_win_id_mutex;
typedef struct s3d_window s3d_window;
typedef struct spew3d_camdata {
    double fov;
    s3d_obj3d **_render_collect_objects_buffer;
    uint32_t _render_collect_objects_alloc;
} spew3d_camdata;

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
S3DHID void *spew3d_scene3d_ObjExtraData(s3d_obj3d *obj);

S3DHID void spew3d_camera_CameraFreeData(
        s3d_obj3d *obj, void *extra
        ) {
    spew3d_camdata *mdata = (
        (spew3d_camdata *)extra
    );
    if (mdata->_render_collect_objects_buffer != NULL) {
        free(mdata->_render_collect_objects_buffer);
    }
    free(mdata);
}

S3DEXP s3d_obj3d *spew3d_camera3d_CreateForScene(
        s3d_scene3d *scene
        ) {
    s3d_obj3d *obj = malloc(spew3d_obj3d_GetStructSize());
    if (!obj)
        return NULL;
    memset(obj, 0, spew3d_obj3d_GetStructSize());
    _spew3d_scene3d_SetKind_nolock(obj, OBJ3D_CAMERA);

    spew3d_camdata *camdata = malloc(sizeof(*camdata));
    if (!camdata) {
        spew3d_obj3d_Destroy(obj);
        return NULL;
    }
    memset(camdata, 0, sizeof(*camdata));
    camdata->fov = 70;
    _spew3d_scene3d_ObjSetExtraData_nolock(
        obj, camdata, spew3d_camera_CameraFreeData
    );

    int result = spew3d_scene3d_AddPreexistingObj(
        scene, obj
    );
    if (!result) {
        spew3d_obj3d_Destroy(obj);
        return NULL;
    }
    assert(spew3d_scene3d_GetStoreByObj3d(obj) != NULL);
    return obj;
}

S3DEXP void spew3d_camera3d_RenderToWindow(
        s3d_obj3d *cam, s3d_window *win
        ) {
    assert(_spew3d_scene3d_GetKind_nolock(cam) == OBJ3D_CAMERA);
    s3dequeue *eq = _s3devent_GetInternalQueue();
    if (!eq)
        return;

    s3devent e = {0};
    e.type = S3DEV_INTERNAL_CMD_CAM3D_DRAWTOWINDOW;
    e.cam3d.obj_ref = cam;
    e.cam3d.win_id = spew3d_window_GetID(win);
    if (!s3devent_q_Insert(eq, &e))
        return;
}

S3DHID int _spew3d_camera3d_ProcessDrawToWindowReq(s3devent *ev) {
    if (!_internal_spew3d_InitSDLGraphics())
        return 0;

    s3d_window *win = _spew3d_window_GetByIDLocked(ev->cam3d.win_id);
    if (!win)
        return 1;

    s3d_obj3d *cam = ev->cam3d.obj_ref;

    #ifndef SPEW3D_OPTION_DISABLE_SDL
    mutex_Release(_win_id_mutex);
    SDL_Renderer *render = NULL;
    spew3d_window_GetSDLWindowAndRenderer(
        win, NULL, &render
        );
    #endif
    s3d_spatialstore3d *store = (
        spew3d_scene3d_GetStoreByObj3d(cam)
    );
    assert(store != NULL);
    spew3d_camdata *cdata = (spew3d_camdata *)(
        spew3d_scene3d_ObjExtraData(cam)
    );
    s3d_obj3d **buf = cdata->_render_collect_objects_buffer;
    uint32_t alloc = cdata->_render_collect_objects_alloc;
    uint32_t count = 0;
    int result = store->IterateAll(
        store, NULL, 0, &buf,
        &alloc, &count
    );
    if (!result) {
        // We're probably out of memory. Not much we can do.
        count = 0;
    }
    cdata->_render_collect_objects_buffer = buf;
    cdata->_render_collect_objects_alloc = alloc;
    if (count <= 0) {
        #ifndef SPEW3D_OPTION_DISABLE_SDL
        mutex_Lock(_win_id_mutex);
        #endif
        return 1;
    }

    #ifndef SPEW3D_OPTION_DISABLE_SDL
    mutex_Lock(_win_id_mutex);
    #endif

    return 1;
}

S3DEXP int spew3d_camera_MainThreadProcessEvent(s3devent *e) {
    assert(thread_InMainThread());

    s3dequeue *eq = _s3devent_GetInternalQueue();

    mutex_Lock(_win_id_mutex);
    if (e->type == S3DEV_INTERNAL_CMD_CAM3D_DRAWTOWINDOW) {
        if (!_spew3d_camera3d_ProcessDrawToWindowReq(e)) {
            mutex_Release(_win_id_mutex);
            s3devent_q_Insert(eq, e);
        }
        mutex_Release(_win_id_mutex);
        return 1;
    }
    mutex_Release(_win_id_mutex);
    return 0;
}

#endif  // SPEW3D_IMPLEMENTATION

