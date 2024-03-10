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

S3DHID s3d_backend_windowing_gputex *
    _internal_spew3d_MainThreadOnly_GetGPUTex_nolock(
        s3d_window *win, s3d_texture_t tex, int withalphachannel
    );;
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
    s3d_equeue *eq = _spew3d_event_GetInternalQueue();
    if (!eq)
        return;

    s3d_event e = {0};
    e.kind = S3DEV_INTERNAL_CMD_CAM3D_DRAWTOWINDOW;
    e.cam3d.obj_ref = cam;
    e.cam3d.win_id = spew3d_window_GetID(win);
    if (!spew3d_event_q_Insert(eq, &e))
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

S3DEXP void _internal_spew3d_camera3d_UpdateRenderPolyData(
        s3d_renderpolygon *rqueue,
        uint32_t index
        ) {
    s3d_pos center;
    center.x = (rqueue[index].vertex_pos[0].x +
        rqueue[index].vertex_pos[1].x +
        rqueue[index].vertex_pos[2].x) / 3.0;
    center.y = (rqueue[index].vertex_pos[0].y +
        rqueue[index].vertex_pos[1].y +
        rqueue[index].vertex_pos[2].y) / 3.0;
    center.z = (rqueue[index].vertex_pos[0].z +
        rqueue[index].vertex_pos[1].z +
        rqueue[index].vertex_pos[2].z) / 3.0;
    rqueue[index].center = center;
    rqueue[index].min_depth = fmin(fmin(
        rqueue[index].vertex_pos[0].x,
        rqueue[index].vertex_pos[1].x),
        rqueue[index].vertex_pos[2].x);
    rqueue[index].max_depth = fmax(fmax(
        rqueue[index].vertex_pos[0].x,
        rqueue[index].vertex_pos[1].x),
        rqueue[index].vertex_pos[2].x);
}

S3DHID static int _spew3d_camera3d_CheckClipped(
        s3d_renderpolygon *poly,
        double pixel_wf, double pixel_hf
        ) {
    if (poly->clipped)
        return 1;
    if (poly->center.x < 0) {
        poly->clipped = 1;
        return 1;
    }
    if (poly->vertex_pos_pixels[0].y < 0 &&
            poly->vertex_pos_pixels[1].y < 0 &&
            poly->vertex_pos_pixels[2].y < 0) {
        poly->clipped = 1;
        return 1;
    }
    if (poly->vertex_pos_pixels[0].z < 0 &&
            poly->vertex_pos_pixels[1].z < 0 &&
            poly->vertex_pos_pixels[2].z < 0) {
        poly->clipped = 1;
        return 1;
    }
    if (poly->vertex_pos_pixels[0].y > pixel_wf &&
            poly->vertex_pos_pixels[1].y > pixel_wf &&
            poly->vertex_pos_pixels[2].y > pixel_wf) {
        poly->clipped = 1;
        return 1;
    }
    if (poly->vertex_pos_pixels[0].z > pixel_hf &&
            poly->vertex_pos_pixels[1].z > pixel_hf &&
            poly->vertex_pos_pixels[2].z > pixel_hf) {
        poly->clipped = 1;
        return 1;
    }
    return 0;
}

S3DHID static int _spew3d_camera3d_SplitPolygonsIfNeeded(
        s3d_obj3d *cam, s3d_transform3d_cam_info *cam_info,
        uint32_t *buf_fill,
        uint32_t pixel_w, uint32_t pixel_h
        ) {
    spew3d_obj3d_LockAccess(cam);
    s3d_camdata *cdata = (s3d_camdata *)(
        _spew3d_scene3d_ObjExtraData_nolock(cam)
    );
    s3d_renderpolygon *polys = cdata->_render_polygon_buffer;
    uint32_t alloc = cdata->_render_polygon_buffer_alloc;
    uint32_t fill = *buf_fill;
    spew3d_obj3d_ReleaseAccess(cam);
    double pixel_wf = pixel_w;
    double pixel_hf = pixel_h;
    double threshold = (pixel_wf + pixel_hf) / 2.0 / 3.0;
    double threshold_fine = (pixel_wf + pixel_hf) / 2.0 / 8.0;

    uint32_t origfill = fill;
    int maxiterations = 4;
    int hadsplit = 1;
    while (hadsplit && maxiterations > 0) {
        maxiterations--;
        hadsplit = 0;
        origfill = fill;
        uint32_t i = 0;
        while (i < origfill) {
            s3d_renderpolygon *p = &polys[i];
            double applied_threshold = threshold;
            if (p->min_depth <= 0)
                applied_threshold = threshold_fine;
            if (!_spew3d_camera3d_CheckClipped(p,
                    pixel_wf, pixel_hf) &&
                    fabs(p->vertex_pos_pixels[0].z -
                        p->vertex_pos_pixels[1].z) >
                        applied_threshold ||
                    fabs(p->vertex_pos_pixels[0].y -
                        p->vertex_pos_pixels[1].y) >
                        applied_threshold ||
                    fabs(p->vertex_pos_pixels[0].x -
                        p->vertex_pos_pixels[1].x) >
                        applied_threshold ||
                    fabs(p->vertex_pos_pixels[0].z -
                        p->vertex_pos_pixels[2].z) >
                        applied_threshold ||
                    fabs(p->vertex_pos_pixels[0].y -
                        p->vertex_pos_pixels[2].y) >
                        applied_threshold ||
                    fabs(p->vertex_pos_pixels[0].x -
                        p->vertex_pos_pixels[2].x) >
                        applied_threshold ||
                    fabs(p->vertex_pos_pixels[1].z -
                        p->vertex_pos_pixels[2].z) >
                        applied_threshold ||
                    fabs(p->vertex_pos_pixels[1].y -
                        p->vertex_pos_pixels[2].y) >
                        applied_threshold ||
                    fabs(p->vertex_pos_pixels[1].x -
                        p->vertex_pos_pixels[2].x) >
                        applied_threshold) {
                // We want to split this one up.
                hadsplit = 1;
                if (fill + 3 > alloc) {
                    uint32_t newalloc = (fill + 3 + 32) * 2;
                    s3d_renderpolygon *newpolys = realloc(
                        polys, sizeof(*newpolys) * newalloc
                    );
                    if (!newpolys) {
                        spew3d_obj3d_LockAccess(cam);
                        cdata->_render_polygon_buffer = polys;
                        cdata->_render_polygon_buffer_alloc = alloc;
                        *buf_fill = fill;
                        spew3d_obj3d_ReleaseAccess(cam);
                        return 0;
                    }
                    polys = newpolys;
                    alloc = newalloc;
                    p = &polys[i];
                }

                s3d_pos sample_v1tov2;
                sample_v1tov2.x = (p->vertex_pos[0].x +
                    p->vertex_pos[1].x) / 2;
                sample_v1tov2.y = (p->vertex_pos[0].y +
                    p->vertex_pos[1].y) / 2;
                sample_v1tov2.z = (p->vertex_pos[0].z +
                    p->vertex_pos[1].z) / 2;
                s3d_point sample_v1tov2_tx;
                s3d_pos sample_v1tov2_pixels;

                s3d_pos sample_v2tov3;
                sample_v2tov3.x = (p->vertex_pos[1].x +
                    p->vertex_pos[2].x) / 2;
                sample_v2tov3.y = (p->vertex_pos[1].y +
                    p->vertex_pos[2].y) / 2;
                sample_v2tov3.z = (p->vertex_pos[1].z +
                    p->vertex_pos[2].z) / 2;
                s3d_point sample_v2tov3_tx;
                s3d_pos sample_v2tov3_pixels;

                s3d_pos sample_v3tov1;
                sample_v3tov1.x = (p->vertex_pos[2].x +
                    p->vertex_pos[0].x) / 2;
                sample_v3tov1.y = (p->vertex_pos[2].y +
                    p->vertex_pos[0].y) / 2;
                sample_v3tov1.z = (p->vertex_pos[2].z +
                    p->vertex_pos[0].z) / 2;
                s3d_point sample_v3tov1_tx;
                s3d_pos sample_v3tov1_pixels;

                spew3d_math3d_sample_polygon_texcoord(
                    cam_info,
                    &p->vertex_pos[0], &p->vertex_pos[1],
                    &p->vertex_pos[2],
                    &p->vertex_texcoord[0], &p->vertex_texcoord[1],
                    &p->vertex_texcoord[2],
                    sample_v1tov2,
                    &sample_v1tov2, &sample_v1tov2_pixels,
                    &sample_v1tov2_tx
                );
                spew3d_math3d_sample_polygon_texcoord(
                    cam_info,
                    &p->vertex_pos[0], &p->vertex_pos[1],
                    &p->vertex_pos[2],
                    &p->vertex_texcoord[0], &p->vertex_texcoord[1],
                    &p->vertex_texcoord[2],
                    sample_v2tov3,
                    &sample_v2tov3, &sample_v2tov3_pixels,
                    &sample_v2tov3_tx
                );
                spew3d_math3d_sample_polygon_texcoord(
                    cam_info,
                    &p->vertex_pos[0], &p->vertex_pos[1],
                    &p->vertex_pos[2],
                    &p->vertex_texcoord[0], &p->vertex_texcoord[1],
                    &p->vertex_texcoord[2],
                    sample_v3tov1,
                    &sample_v3tov1, &sample_v3tov1_pixels,
                    &sample_v3tov1_tx
                );

                s3d_renderpolygon *p2 = &polys[fill];
                s3d_renderpolygon *p3 = &polys[fill + 1];
                s3d_renderpolygon *p4 = &polys[fill + 2];
                memcpy(p2, p, sizeof(*p2));
                memcpy(p3, p, sizeof(*p3));
                memcpy(p4, p, sizeof(*p4));

                p2->vertex_pos[0] = sample_v1tov2;
                p2->vertex_pos[1] = p->vertex_pos[1];
                p2->vertex_pos[2] = sample_v2tov3;
                p2->vertex_pos_pixels[0] = sample_v1tov2_pixels;
                p2->vertex_pos_pixels[1] = (
                    p->vertex_pos_pixels[1]
                );
                p2->vertex_pos_pixels[2] = sample_v2tov3_pixels;
                p2->vertex_texcoord[0] = sample_v1tov2_tx;
                p2->vertex_texcoord[1] = p->vertex_texcoord[1];
                p2->vertex_texcoord[2] = sample_v2tov3_tx;
                _internal_spew3d_camera3d_UpdateRenderPolyData(
                    polys, fill
                );
                fill++;
                assert(fill <= alloc);

                p3->vertex_pos[0] = sample_v2tov3;
                p3->vertex_pos[1] = p->vertex_pos[2];
                p3->vertex_pos[2] = sample_v3tov1;
                p3->vertex_pos_pixels[0] = sample_v2tov3_pixels;
                p3->vertex_pos_pixels[1] = (
                    p->vertex_pos_pixels[2]
                );
                p3->vertex_pos_pixels[2] = sample_v3tov1_pixels;
                p3->vertex_texcoord[0] = sample_v2tov3_tx;
                p3->vertex_texcoord[1] = p->vertex_texcoord[2];
                p3->vertex_texcoord[2] = sample_v3tov1_tx;
                _internal_spew3d_camera3d_UpdateRenderPolyData(
                    polys, fill
                );
                fill++;
                assert(fill <= alloc);

                p4->vertex_pos[0] = sample_v2tov3;
                p4->vertex_pos[1] = sample_v3tov1;
                p4->vertex_pos[2] = sample_v1tov2;
                p4->vertex_pos_pixels[0] = sample_v2tov3_pixels;
                p4->vertex_pos_pixels[1] = sample_v3tov1_pixels;
                p4->vertex_pos_pixels[2] = sample_v1tov2_pixels;
                p4->vertex_texcoord[0] = sample_v2tov3_tx;
                p4->vertex_texcoord[1] = sample_v3tov1_tx;
                p4->vertex_texcoord[2] = sample_v1tov2_tx;
                _internal_spew3d_camera3d_UpdateRenderPolyData(
                    polys, fill
                );
                fill++;
                assert(fill <= alloc);

                p->vertex_pos[1] = sample_v1tov2;
                p->vertex_pos[2] = sample_v3tov1;
                p->vertex_pos_pixels[1] = sample_v1tov2_pixels;
                p->vertex_pos_pixels[2] = sample_v3tov1_pixels;
                p->vertex_texcoord[1] = sample_v1tov2_tx;
                p->vertex_texcoord[2] = sample_v3tov1_tx;
                _internal_spew3d_camera3d_UpdateRenderPolyData(
                    polys, i
                );
            }
            i++;
        }
    }
    spew3d_obj3d_LockAccess(cam);
    cdata->_render_polygon_buffer = polys;
    cdata->_render_polygon_buffer_alloc = alloc;
    *buf_fill = fill;
    spew3d_obj3d_ReleaseAccess(cam);
    return 1;
}

S3DHID int _spew3d_camera3d_ProcessDrawToWindowReq(
        s3d_event *ev
        ) {
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
    if (pixel_w < 1) pixel_w = 1;
    if (pixel_h < 1) pixel_h = 1;
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
            queuefill++;
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
            queuefill++;
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
        } else if (queue[i].kind == RENDERENTRY_LVLBOX) {
            rinfo.dynlight_mode = DLRD_LIT_FLAT;
            int try_add = spew3d_lvlbox_Transform(
                queue[i].renderlvlbox.lvlbox,
                &queue[i].renderlvlbox.world_pos,
                &queue[i].renderlvlbox.world_rotation,
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
        "Now splitting geometry, "
        "polygon queue length: %d\n", polybuf_fill);
    #endif
    _spew3d_camera3d_SplitPolygonsIfNeeded(
        cam, &cinfo, &polybuf_fill, pixel_w, pixel_h
    );
    spew3d_obj3d_LockAccess(cam);
    polybuf = cdata->_render_polygon_buffer;
    polybuf_alloc = cdata->_render_polygon_buffer_alloc;
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
    s3d_backend_windowing_wininfo *backend_winfo;
    s3d_backend_windowing *backend = spew3d_window_GetBackend(
        win, &backend_winfo
    );
    i = 0;
    while (i < polybuf_fill) {
        // If the polygon is too far behind the camera, clip it:
        if (polybuf[i].center.x <= 0) {
            i++;
            continue;
        }

        s3d_renderpolygon *p = &polybuf[i];
        s3d_color colors[3] = {0};
        colors[0].red = 1.0;
        colors[0].green = 1.0;
        colors[0].blue = 1.0;
        colors[0].alpha = 1.0;
        colors[1].red = 1.0;
        colors[1].green = 1.0;
        colors[1].blue = 1.0;
        colors[1].alpha = 1.0;
        colors[2].red = 1.0;
        colors[2].green = 1.0;
        colors[2].blue = 1.0;
        colors[2].alpha = 1.0;

        s3d_backend_windowing_gputex *tex = NULL;
        if (p->polygon_texture != 0) {
            mutex_Lock(_texlist_mutex);
            tex = _internal_spew3d_MainThreadOnly_GetGPUTex_nolock(
                win, p->polygon_texture, 1
            );
            mutex_Release(_texlist_mutex);
            if (tex == NULL) {
                // FIXME: Do we want some placeholder graphics here?
            }
        }
        backend->DrawPolygonAtPixels(
            backend, win, backend_winfo,
            tex, &p->vertex_pos_pixels[0],
            &p->vertex_texcoord[0],
            (s3d_color *)colors
        );
        i++;
    }
    #else
    // FIXME: Eventually implement custom software renderer
    #endif

    mutex_Lock(_win_id_mutex);

    return 1;
}

S3DEXP int spew3d_camera_InternalMainThreadProcessEvent(
        s3d_event *e
        ) {
    assert(thread_InMainThread());

    s3d_equeue *eq = _spew3d_event_GetInternalQueue();

    mutex_Lock(_win_id_mutex);
    if (e->kind == S3DEV_INTERNAL_CMD_CAM3D_DRAWTOWINDOW) {
        if (!_spew3d_camera3d_ProcessDrawToWindowReq(e)) {
            mutex_Release(_win_id_mutex);
            spew3d_event_q_Insert(eq, e);
        }
        mutex_Release(_win_id_mutex);
        return 1;
    }
    mutex_Release(_win_id_mutex);
    return 0;
}

#endif  // SPEW3D_IMPLEMENTATION

