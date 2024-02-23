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

#include <assert.h>
#ifndef SPEW3D_OPTION_DISABLE_SDL
#include <SDL2/SDL.h>
#endif
#include <string.h>
#include <stdint.h>

extern s3d_mutex *_win_id_mutex;
extern s3d_mutex *_texlist_mutex;
typedef struct s3d_window s3d_window;

#define RENDERENTRY_INVALID 0
#define RENDERENTRY_SPRITE3D 1
#define RENDERENTRY_MESH 2
#define RENDERENTRY_LIGHT 3
#define RENDERENTRY_LVLBOX 4

typedef struct s3d_queuedrenderentry {
    int kind;
    union {
        struct rendermesh {
            s3d_geometry *geom;
            s3d_bone *bone;

            s3d_rotation world_rotation;
            s3d_pos world_pos;
            double world_max_extent;
        } rendermesh;
        struct rendersprite3d {
            s3d_rotation world_rotation;
            s3d_pos world_pos;
        } rendersprite3d;
        struct renderlvlbox {
            s3d_lvlbox *lvlbox;
            s3d_rotation world_rotation;
            s3d_pos world_pos;
        } renderlvlbox;
    };
} s3d_queuedrenderentry;

typedef struct s3d_camdata {
    double fov;
    s3d_obj3d **_render_collect_objects_buffer;
    uint32_t _render_collect_objects_alloc;
    s3d_queuedrenderentry *_render_queue_buffer;
    uint32_t _render_queue_buffer_alloc;
    s3d_renderpolygon *_render_polygon_buffer;
    uint32_t _render_polygon_buffer_alloc;
    s3d_sortstructcache *_render_sort_cache;
} s3d_camdata;

#ifndef SPEW3D_OPTION_DISABLE_SDL
S3DHID SDL_Texture *_internal_spew3d_MainThreadOnly_GetTex_nolock(
    s3d_window *win, s3d_texture_t tex, int withalphachannel
);
#endif
S3DHID s3d_scenecolorinfo spew3d_scene3d_GetColorInfo_nolock(
    s3d_scene3d *sc
);
S3DEXP s3d_scene3d *spew3d_obj3d_GetScene_nolock(
    s3d_obj3d *obj
);
S3DEXP int _spew3d_obj3d_GetWasDeleted_nolock(
    s3d_obj3d *obj
);
S3DHID void _spew3d_scene3d_GetObjLvlbox_nolock(
    s3d_obj3d *obj, s3d_lvlbox **lvlbox
);
S3DHID void _spew3d_scene3d_GetObjMeshes_nolock(
    s3d_obj3d *obj, s3d_geometry **first_mesh,
    s3d_geometry ***extra_meshes,
    uint32_t *extra_meshes_count
);
S3DEXP double spew3d_obj3d_GetOuterMaxExtentRadius_nolock(
    s3d_obj3d *obj
);
S3DHID s3d_pos spew3d_obj3d_GetPos_nolock(s3d_obj3d *obj);
S3DHID s3d_rotation spew3d_obj3d_GetRotation_nolock(s3d_obj3d *obj);
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
    s3d_camdata *mdata = (
        (s3d_camdata *)extra
    );
    if (mdata->_render_collect_objects_buffer != NULL) {
        free(mdata->_render_collect_objects_buffer);
    }
    if (mdata->_render_queue_buffer) {
        free(mdata->_render_queue_buffer);
    }
    if (mdata->_render_sort_cache) {
        s3d_itemsort_FreeCache(mdata->_render_sort_cache);
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

    s3d_camdata *camdata = malloc(sizeof(*camdata));
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

S3DEXP void spew3d_camera3d_SetFOV(
        s3d_obj3d *cam, double fov
        ) {
    assert(_spew3d_scene3d_GetKind_nolock(cam) == OBJ3D_CAMERA);
    s3d_camdata *camdata = _spew3d_scene3d_ObjExtraData_nolock(
        cam
    );
    camdata->fov = fov;
}

S3DEXP void spew3d_camera3d_RenderToWindow(
        s3d_obj3d *cam, s3d_window *win
        ) {
    assert(_spew3d_scene3d_GetKind_nolock(cam) == OBJ3D_CAMERA);
    s3dequeue *eq = _s3devent_GetInternalQueue();
    if (!eq)
        return;

    s3devent e = {0};
    e.kind = S3DEV_INTERNAL_CMD_CAM3D_DRAWTOWINDOW;
    e.cam3d.obj_ref = cam;
    e.cam3d.win_id = spew3d_window_GetID(win);
    if (!s3devent_q_Insert(eq, &e))
        return;
}

S3DHID void _spew3d_window_ExtractCanvasSize_nolock(
    s3d_window *win, uint32_t *out_w, uint32_t *out_h
);

S3DHID static int _depthCompareRenderPolygons(
        void *item1, void *item2
        ) {
    s3d_renderpolygon *entry1 = item1;
    s3d_renderpolygon *entry2 = item2;
    /*printf("comparing poly %p <-> %p, "
        "entry1->vertex_pos[0].x = %f "
        "entry2->vertex_pos[0].x = %f\n",
        entry1->vertex_pos_pixels[0].x, entry2->vertex_pos_pixels[0].x);*/
    double depth1 = (entry1->min_depth + entry1->max_depth) / 2;
    double depth2 = (entry2->min_depth + entry2->max_depth) / 2;
    if (depth1 < depth2)
        return 1;
    if (depth1 > depth2)
        return -1;
    return 0;
}

S3DHID int _spew3d_camera3d_ProcessDrawToWindowReq(s3devent *ev) {
    if (!_internal_spew3d_InitSDLGraphics())
        return 0;

    s3d_window *win = _spew3d_window_GetByIDLocked(ev->cam3d.win_id);
    if (!win)
        return 1;

    s3d_obj3d *cam = ev->cam3d.obj_ref;
    spew3d_obj3d_LockAccess(cam);
    s3d_pos cam_pos = spew3d_obj3d_GetPos_nolock(cam);
    s3d_rotation cam_rot = spew3d_obj3d_GetRotation_nolock(cam);
    uint32_t pixel_w = 1;
    uint32_t pixel_h = 1;
    _spew3d_window_ExtractCanvasSize_nolock(
        win, &pixel_w, &pixel_h
    );
    s3d_camdata *cdata = (s3d_camdata *)(
        _spew3d_scene3d_ObjExtraData_nolock(cam)
    );
    double fov = cdata->fov;
    spew3d_obj3d_ReleaseAccess(cam);

    mutex_Release(_win_id_mutex);
    #ifndef SPEW3D_OPTION_DISABLE_SDL
    SDL_Renderer *render = NULL;
    spew3d_window_GetSDLWindowAndRenderer(
        win, NULL, &render
        );
    #endif

    // First, collect whatever we even want to render:
    s3d_spatialstore3d *store = (
        spew3d_scene3d_GetStoreByObj3d(cam)
    );
    assert(store != NULL);
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
    // For performance reasons, make a separate copy to process
    // culling first, so that things like physics and AI can
    // continue without us blocking them.
    // XXX: NOTE: This code relies on the assumption that
    // an object's mesh will not change while the object is
    // in the scene, so we access the meshes without locks
    // later.
    s3d_queuedrenderentry *queue = cdata->_render_queue_buffer;
    uint32_t queue_alloc = cdata->_render_queue_buffer_alloc;
    if (count <= 0) {
        mutex_Lock(_win_id_mutex);
        return 1;
    }
    spew3d_obj3d_LockAccess(cam);  // Should also lock scene.
    uint32_t queuefill = 0;
    uint32_t i = 0;
    while (i < count) {
        s3d_obj3d *obj = buf[i];
        if (_spew3d_obj3d_GetWasDeleted_nolock(obj)) {
            i++;
            continue;
        }
        int kind = _spew3d_scene3d_GetKind_nolock(obj);
        if (kind != OBJ3D_MESH && kind != OBJ3D_SPRITE3D &&
                kind != OBJ3D_LVLBOX
                ) {
            i++;
            continue;
        }
        if (queuefill + 1 > queue_alloc) {
            uint32_t new_alloc = (16 + queuefill + 1) * 2;
            s3d_queuedrenderentry *newqueue = realloc(
                queue, sizeof(*queue) * new_alloc
            );
            if (!newqueue) {
                // Out of memory, we can't render like this.
                cdata->_render_queue_buffer = queue;
                cdata->_render_queue_buffer_alloc = queue_alloc;
                spew3d_obj3d_ReleaseAccess(cam);
                mutex_Lock(_win_id_mutex);
                return 1;
            }
            queue = newqueue;
            queue_alloc = new_alloc;
        }
        s3d_pos pos = spew3d_obj3d_GetPos_nolock(obj);
        s3d_rotation rot = (
            spew3d_obj3d_GetRotation_nolock(obj)
        );
        if (kind == OBJ3D_SPRITE3D) {
            // FIXME. Actually save some info on sprite, somehow.
            memset(&queue[queuefill], 0, sizeof(queue[queuefill]));
            queue[queuefill].kind = RENDERENTRY_SPRITE3D;
            queue[queuefill].rendersprite3d.world_pos = pos;
            queue[queuefill].rendersprite3d.world_rotation = rot;
            i++;
            continue;
        } else if (kind == OBJ3D_LVLBOX) {
            memset(&queue[queuefill], 0, sizeof(queue[queuefill]));
            queue[queuefill].kind = RENDERENTRY_LVLBOX;
            _spew3d_scene3d_GetObjLvlbox_nolock(
                obj, &queue[queuefill].renderlvlbox.lvlbox
            );
            queue[queuefill].renderlvlbox.world_pos = pos;
            queue[queuefill].renderlvlbox.world_rotation = rot;
            i++;
            continue;
        }
        assert(kind == OBJ3D_MESH);

        // Collect the extra meshes, if any:
        s3d_geometry *first_mesh = NULL;
        s3d_geometry **extra_meshes = NULL;
        uint32_t extra_meshes_count = 0;
        _spew3d_scene3d_GetObjMeshes_nolock(
            obj, &first_mesh, &extra_meshes,
            &extra_meshes_count
        );
        if (extra_meshes_count > 0 &&
                queuefill + extra_meshes_count > queue_alloc) {
            uint32_t new_alloc = (
                16 + queuefill +
                extra_meshes_count
            ) * 2;
            s3d_queuedrenderentry *newqueue = realloc(
                queue, sizeof(*queue) * new_alloc
            );
            if (!newqueue) {
                // Out of memory, we can't render like this.
                spew3d_obj3d_ReleaseAccess(cam);
                mutex_Lock(_win_id_mutex);
                return 1;
            }
            queue = newqueue;
            queue_alloc = new_alloc;
        }
        memset(&queue[queuefill], 0, sizeof(queue[queuefill]));
        queue[queuefill].kind = RENDERENTRY_MESH;
        queue[queuefill].rendermesh.geom = first_mesh;
        queue[queuefill].rendermesh.world_pos = pos;
        queue[queuefill].rendermesh.world_rotation = rot;
        queue[queuefill].rendermesh.world_max_extent =
            spew3d_obj3d_GetOuterMaxExtentRadius_nolock(obj);
        int origindex = queuefill;
        queuefill++;
        uint32_t j = 0;
        while (j < extra_meshes_count) {
            memcpy(&queue[queuefill],
                &queue[origindex], sizeof(queue[queuefill]));
            queue[queuefill].rendermesh.geom = extra_meshes[j];
            j++;
        }
        i++;
    }
    cdata->_render_queue_buffer = queue;
    cdata->_render_queue_buffer_alloc = queue_alloc;

    // Prepare the polygon buffer and compute general scene info:
    s3d_geometryrenderlightinfo rinfo = {0};
    memset(&rinfo, 0, sizeof(rinfo));
    rinfo.ambient_emit.red = 1.0;
    rinfo.ambient_emit.green = 1.0;
    rinfo.ambient_emit.blue = 1.0;
    s3d_scene3d *sc = spew3d_obj3d_GetScene_nolock(cam);
    assert(sc != NULL);
    s3d_scenecolorinfo coloring = spew3d_scene3d_GetColorInfo_nolock(
        sc
    );
    if (coloring.ambient_emit.red > 0 ||
                coloring.ambient_emit.green > 0 ||
                coloring.ambient_emit.blue > 0) {
        rinfo.ambient_emit.red = (
            coloring.ambient_emit.red
        );
        rinfo.ambient_emit.green = (
            coloring.ambient_emit.green
        );
        rinfo.ambient_emit.blue = (
            coloring.ambient_emit.blue
        );
    }
    s3d_renderpolygon *polybuf = cdata->_render_polygon_buffer;
    uint32_t polybuf_alloc = cdata->_render_polygon_buffer_alloc;
    spew3d_obj3d_ReleaseAccess(cam);

    // Compute fov (we don't need a scene lock for that):
    s3d_transform3d_cam_info cinfo = {0};
    cinfo.cam_pos = cam_pos;
    cinfo.cam_rotation = cam_rot;
    cinfo.viewport_pixel_width = pixel_w;
    cinfo.viewport_pixel_height = pixel_h;
    spew3d_math3d_split_fovs_from_fov(
        fov, pixel_w, pixel_h,
        &cinfo.cam_horifov,
        &cinfo.cam_vertifov
    );

    // Compute actual polygons to sort them later:
    uint32_t polybuf_fill = 0;
    i = 0;
    while (i < queuefill) {
        if (queue[i].kind == RENDERENTRY_MESH) {
            rinfo.dynlight_mode = DLRD_LIT_FLAT;
            int try_add = spew3d_geometry_Transform(
                queue[i].rendermesh.geom,
                &queue[i].rendermesh.world_pos,
                &queue[i].rendermesh.world_rotation,
                &cinfo,
                &rinfo, &polybuf, &polybuf_fill, &polybuf_alloc
            );
            if (!try_add) {
                // Ran out of memory. Not much we can do.
                i += 1;
            }
        }
        i++;
    }
    spew3d_obj3d_LockAccess(cam);
    cdata->_render_polygon_buffer = polybuf;
    cdata->_render_polygon_buffer_alloc = polybuf_alloc;
    spew3d_obj3d_ReleaseAccess(cam);

    #if defined(DEBUG_SPEW3D_RENDER3D)
    printf("spew3d_camera3d.c: debug: "
        "Now sorting geometry, "
        "polygon queue length: %d\n", polybuf_fill);
    #endif
    if (!cdata->_render_sort_cache) {
        cdata->_render_sort_cache = s3d_itemsort_CreateCache();
        if (!cdata->_render_sort_cache) {
            // We can't do much about this.
        }
    }
    int sort_result = s3d_itemsort_Do(
        polybuf, polybuf_fill * sizeof(polybuf[0]),
        sizeof(polybuf[0]),
        &_depthCompareRenderPolygons,
        cdata->_render_sort_cache,
        NULL, NULL
    );
    if (sort_result == 0) {
        #if defined(DEBUG_SPEW3D_RENDER3D)
        printf("spew3d_camera3d.c: debug: "
            "Sorting failed.\n");
        #endif
    }
    #ifndef SPEW3D_OPTION_DISABLE_SDL
    #if defined(DEBUG_SPEW3D_RENDER3D)
    printf("spew3d_camera3d.c: debug: "
        "SDL2 render of geometry, "
        "polygon queue length: %d\n", polybuf_fill);
    #endif
    SDL_SetRenderDrawColor(render, 255, 255, 255, 255);
    SDL_SetRenderDrawBlendMode(render, SDL_BLENDMODE_BLEND);
    i = 0;
    while (i < polybuf_fill) {
        s3d_renderpolygon *p = &polybuf[i];
        SDL_Vertex vertex_1 = {
            {p->vertex_pos_pixels[0].y,
             p->vertex_pos_pixels[0].z},
            {255, 255, 255, 255},
            {p->vertex_texcoord[0].x, p->vertex_texcoord[0].y}
        };
        SDL_Vertex vertex_2 = {
            {p->vertex_pos_pixels[1].y,
             p->vertex_pos_pixels[1].z},
            {255, 255, 255, 255},
            {p->vertex_texcoord[1].x, p->vertex_texcoord[1].y}
        };
        SDL_Vertex vertex_3 = {
            {p->vertex_pos_pixels[2].y,
             p->vertex_pos_pixels[2].z},
            {255, 255, 255, 255},
            {p->vertex_texcoord[2].x, p->vertex_texcoord[2].y}
        };
        SDL_Vertex vertices[] = {
            vertex_1,
            vertex_2,
            vertex_3
        };
        SDL_Texture *tex = NULL;
        if (p->polygon_texture != 0) {
            mutex_Lock(_texlist_mutex);
            tex = _internal_spew3d_MainThreadOnly_GetTex_nolock(
                win, p->polygon_texture, 1
            );
            mutex_Release(_texlist_mutex);
            if (tex == NULL) {
                // Do we want some placeholder graphics here?
            }
        }
        SDL_RenderGeometry(render, tex, vertices, 3, NULL, 0);
        i++;
    }
    #else
    // FIXME: Eventually implement custom software renderer
    #endif

    mutex_Lock(_win_id_mutex);

    return 1;
}

S3DEXP int spew3d_camera_MainThreadProcessEvent(s3devent *e) {
    assert(thread_InMainThread());

    s3dequeue *eq = _s3devent_GetInternalQueue();

    mutex_Lock(_win_id_mutex);
    if (e->kind == S3DEV_INTERNAL_CMD_CAM3D_DRAWTOWINDOW) {
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

