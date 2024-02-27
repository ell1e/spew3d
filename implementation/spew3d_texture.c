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

#ifdef SPEW3D_IMPLEMENTATION

#include <assert.h>
#include <stdint.h>
#include <string.h>
#ifndef SPEW3D_OPTION_DISABLE_SDL
#include <SDL2/SDL.h>
#endif
#include <unistd.h>

static inline uint16_t spew3d_simplehash(const char *k);

// Some global variables:
uint64_t _internal_spew3d_texlist_count;
s3d_texture_info *_internal_spew3d_texlist;
s3d_mutex *_texlist_mutex = NULL;

// Global hash map:
#define SPEW3D_TEXLIST_IDHASHMAP_SIZE 2048
typedef struct spew3d_texlist_idhashmap_bucket
    spew3d_texlist_idhashmap_bucket;
typedef struct spew3d_texlist_idhashmap_bucket {
    uint64_t texlist_slot_idx;
    spew3d_texlist_idhashmap_bucket *next;
} spew3d_texlist_idhashmap_bucket;
spew3d_texlist_idhashmap_bucket
    **_internal_spew3d_texlist_hashmap = NULL;

// Extra info struct:
typedef struct spew3d_texture_extrainfo {
    s3d_resourceload_job *loadingjob;
    s3d_texture_info *parent;
    uint8_t editlocked;
    uint64_t editlocked_id;

    char *pixels;
    uint32_t width, height;
    int forcenosdl;
    uint64_t last_use_ts;

    #ifndef SPEW3D_OPTION_DISABLE_SDL
    SDL_Texture *sdltexture_alpha, *sdltexture_noalpha;
    #endif
} spew3d_texture_extrainfo;

static char *_internal_tex_get_buf = NULL;
static uint32_t _internal_tex_get_buf_size = 0;

#if !defined(SPEW3D_OPTION_DISABLE_SDL) &&\
        defined(SPEW3D_OPTION_DISABLE_SDL_HEADER)
// This won't be in the header, so define it here:
S3DEXP void spew3d_window_GetSDLWindowAndRenderer(
    s3d_window *win, SDL_Window **out_w,
    SDL_Renderer **out_r
);
#endif

static void __attribute__((constructor)) _internal_spew3d_ensure_texhash() {
    if (_internal_spew3d_texlist_hashmap != NULL)
        return;

    _internal_spew3d_texlist_hashmap = malloc(
        sizeof(*_internal_spew3d_texlist_hashmap) *
        SPEW3D_TEXLIST_IDHASHMAP_SIZE
    );
    if (!_internal_spew3d_texlist_hashmap) {
        fprintf(stderr, "spew3d_texture.c: error: "
            "Failed to allocate _internal_spew3d_texlist_hashmap.\n");
        _exit(1);
    }
    memset(_internal_spew3d_texlist_hashmap, 0,
        sizeof(*_internal_spew3d_texlist_hashmap) *
        SPEW3D_TEXLIST_IDHASHMAP_SIZE
    );

    _texlist_mutex = mutex_Create();
    if (!_texlist_mutex) {
        fprintf(stderr, "spew3d_texture.c: error: "
            "Failed to allocate tex list access mutex.\n");
        _exit(1);
    }
}

S3DHID s3d_texture_info *_internal_spew3d_texinfo_nolock(
        s3d_texture_t id
        ) {
    assert(id > 0 && id <= _internal_spew3d_texlist_count);
    return &_internal_spew3d_texlist[id - 1];
}

S3DEXP void spew3d_texinfo(
        s3d_texture_t id, s3d_texture_info *writeto
        ) {
    mutex_Lock(_texlist_mutex);
    assert(id > 0 && id <= _internal_spew3d_texlist_count);
    s3d_texture_info *i = &_internal_spew3d_texlist[id - 1];
    memcpy(i, writeto, sizeof(*writeto));
    mutex_Release(_texlist_mutex);
}

static inline spew3d_texture_extrainfo *spew3d_extrainfo(
        s3d_texture_t tid
        ) {
    assert(mutex_IsLocked(_texlist_mutex));
    s3d_texture_info *tinfo = _internal_spew3d_texinfo_nolock(tid);
    return ((spew3d_texture_extrainfo *)
        tinfo->_internal);
}

static int _spew3d_check_texidstring_used(
        const char *id) {
    assert(mutex_IsLocked(_texlist_mutex));
    uint16_t hash = spew3d_simplehash(id);
    spew3d_texlist_idhashmap_bucket *bucket =
        _internal_spew3d_texlist_hashmap[hash %
        SPEW3D_TEXLIST_IDHASHMAP_SIZE];
    while (bucket != NULL) {
        const uint64_t idx = bucket->texlist_slot_idx;
        assert(id >= 0 && idx < _internal_spew3d_texlist_count);
        if (_internal_spew3d_texlist[idx].idstring != NULL &&
                strcmp(_internal_spew3d_texlist[idx].idstring,
                    id) == 0) {
            return 1;
        }
        bucket = bucket->next;
    }
    return 0;
}

static int _internal_spew3d_ForceLoadTexture(s3d_texture_t tid) {
    assert(mutex_IsLocked(_texlist_mutex));
    s3d_texture_info *tinfo = _internal_spew3d_texinfo_nolock(tid);
    spew3d_texture_extrainfo *extrainfo = (
        spew3d_extrainfo(tid)
    );

    assert(tinfo != NULL && tinfo->idstring != NULL);
    if (tinfo->loaded || (!tinfo->correspondstofile &&
            !tinfo->diskpath))
        return 1;
    if (tinfo->loadingfailed)
        return 0;
    if (extrainfo->loadingjob != NULL &&
            s3d_resourceload_IsDone(extrainfo->loadingjob)) {
        s3d_resourceload_result r = {0};
        if (!s3d_resourceload_ExtractResult(
                extrainfo->loadingjob, &r, NULL)) {
            assert(r.resource_image.pixels == NULL);
            extrainfo->pixels = NULL;
            tinfo->loadingfailed = 1;
            s3d_resourceload_DestroyJob(extrainfo->loadingjob);
            extrainfo->loadingjob = NULL;
            return 0;
        }
        extrainfo->pixels = r.resource_image.pixels;
        extrainfo->width = r.resource_image.w;
        extrainfo->height = r.resource_image.h;
        assert(extrainfo->pixels != NULL);
        #if defined(DEBUG_SPEW3D_TEXTURE)
        fprintf(stderr,
            "spew3d_texture.c: debug: "
            "_internal_spew3d_texture_ForceLoadTexture(): "
            "loading done\n");
        #endif
        tinfo->loaded = 1;
        s3d_resourceload_DestroyJob(extrainfo->loadingjob);
        extrainfo->loadingjob = NULL;
        return 1;
    }

    if (extrainfo->loadingjob == NULL) {
        #if defined(DEBUG_SPEW3D_TEXTURE)
        fprintf(stderr,
            "spew3d_texture.c: debug: "
            "_internal_spew3d_texture_ForceLoadTexture(): "
            "now creating a job.\n");
        #endif
        extrainfo->loadingjob = s3d_resourceload_NewJob(
            tinfo->diskpath, RLTYPE_IMAGE, tinfo->vfsflags
        );
        if (!extrainfo->loadingjob) {
            tinfo->loadingfailed = 1;
            return 0;
        }
    }

    return 0;
}

#ifndef SPEW3D_OPTION_DISABLE_SDL
S3DHID int _internal_spew3d_TextureToGPU(
        s3d_window *win,
        s3d_texture_t tid, int alpha,
        SDL_Texture **out_tex
        ) {
    assert(mutex_IsLocked(_texlist_mutex));
    if (!_internal_spew3d_ForceLoadTexture(tid))
        return 0;
    s3d_texture_info *tinfo = _internal_spew3d_texinfo_nolock(tid);
    if (tinfo->loadingfailed)
        return 0;
    assert(tinfo != NULL && tinfo->idstring != NULL);
    assert(tinfo->loaded);
    spew3d_texture_extrainfo *extrainfo = (
        spew3d_extrainfo(tid)
    );
    assert(extrainfo != NULL && extrainfo->pixels != NULL);
    if (extrainfo->loadingjob != NULL)
        return 0;

    if (extrainfo->forcenosdl)
        return 1;

    SDL_Renderer *renderer = NULL;
    spew3d_window_GetSDLWindowAndRenderer(
        win, NULL, &renderer
    );
    if (alpha && extrainfo->sdltexture_alpha) {
        *out_tex = extrainfo->sdltexture_alpha;
        return 1;
    } else if (!alpha && extrainfo->sdltexture_noalpha) {
        *out_tex = extrainfo->sdltexture_noalpha;
        return 1;
    }

    SDL_Surface *s = SDL_CreateRGBSurfaceFrom(
        extrainfo->pixels, extrainfo->width,
        extrainfo->height,
        32, extrainfo->width * 4, 0x000000ff,
        0x0000ff00, 0x00ff0000,
        0xff000000
    );
    if (!s)
        return 0;
    if (!alpha) {
        SDL_LockSurface(s);
        uint32_t pitch = s->pitch;
        uint8_t *p = (uint8_t *)s->pixels;
        const uint32_t p2end_offset = s->w *4;
        uint32_t y = 0;
        while (y < s->h) {
            uint8_t *p2 = p;
            uint8_t *p2end = p2 + p2end_offset;
            while (p2 != p2end) {
                p2[3] = 255;
                p2 += 4;
            }
            p += pitch;
            y++;
        }
        SDL_UnlockSurface(s);
    }
    SDL_SetHintWithPriority(SDL_HINT_RENDER_SCALE_QUALITY, "0",
        SDL_HINT_OVERRIDE); 
    if (alpha) {
        extrainfo->sdltexture_alpha = SDL_CreateTextureFromSurface(
            renderer, s
        );
        if (!extrainfo->sdltexture_alpha)
            return 0;
        *out_tex = extrainfo->sdltexture_alpha;
        return 1;
    } else {
        extrainfo->sdltexture_noalpha = SDL_CreateTextureFromSurface(
            renderer, s
        );
        if (!extrainfo->sdltexture_noalpha)
            return 0;
        *out_tex = extrainfo->sdltexture_noalpha;
        return 1;
    }
}
#endif  // #ifndef SPEW3D_OPTION_DISABLE_SDL

S3DEXP const char *spew3d_texture_GetReadonlyPixels(
        s3d_texture_t tid
        ) {
    mutex_Lock(_texlist_mutex);
    s3d_texture_info *tinfo = _internal_spew3d_texinfo_nolock(tid);
    if (!_internal_spew3d_ForceLoadTexture(tid)) {
        mutex_Release(_texlist_mutex);
        return NULL;
    }
    const char *pixels = spew3d_extrainfo(tid)->pixels;
    mutex_Release(_texlist_mutex);
    return pixels;
}

uint64_t _last_lock_req_id = 0;

S3DEXP char *spew3d_texture_UnlockPixelsToEdit(
        s3d_texture_t tid
        ) {
    s3d_equeue *eq = _spew3d_event_GetInternalQueue();
    assert(eq != NULL);

    mutex_Lock(_texlist_mutex);
    _last_lock_req_id++;
    uint64_t lock_req_id = _last_lock_req_id;
    s3d_texture_info *tinfo = _internal_spew3d_texinfo_nolock(tid);
    spew3d_texture_extrainfo *einfo = spew3d_extrainfo(tid);
    if (!_internal_spew3d_ForceLoadTexture(tid)) {
        mutex_Release(_texlist_mutex);
        return NULL;
    }
    assert(!tinfo->correspondstofile);
    assert(tinfo->loaded);
    while (1) {
        if (einfo->editlocked) {
            mutex_Release(_texlist_mutex);
            spew3d_time_Sleep(10);
            mutex_Lock(_texlist_mutex);
            continue;
        }
        einfo->editlocked = 1;
        einfo->editlocked_id = lock_req_id;
    }
    char *pixels = einfo->pixels;
    mutex_Release(_texlist_mutex);
    return pixels;
}

S3DEXP void spew3d_texture_LockPixelsToFinishEdit(
        s3d_texture_t tid
        ) {
    mutex_Lock(_texlist_mutex);
    _last_lock_req_id++;
    uint64_t lock_req_id = _last_lock_req_id;
    s3d_event e = {0};
    e.kind = S3DEV_INTERNAL_CMD_TEXTURELOCK_LOCKPIXELSTOFINISH;
    e.texturelock.tid = tid;
    e.texturelock.lock_request_id = lock_req_id;
    mutex_Release(_texlist_mutex);

    _spew3d_event_q_InsertForce(_spew3d_event_GetInternalQueue(), &e);
}

S3DHID int _spew3d_window_ProcessTexLockPixelsReq(s3d_event *ev) {
    assert(mutex_IsLocked(_texlist_mutex));
    spew3d_texture_extrainfo *einfo = spew3d_extrainfo(
        ev->texturelock.tid);
    #ifndef NDEBUG
    assert(!einfo->editlocked);
    #endif
    einfo->editlocked = 0;
    #ifndef SPEW3D_OPTION_DISABLE_SDL
    if (einfo->sdltexture_alpha) {
        SDL_DestroyTexture(einfo->sdltexture_alpha);
        einfo->sdltexture_alpha = NULL;
    }
    if (einfo->sdltexture_noalpha) {
        SDL_DestroyTexture(einfo->sdltexture_noalpha);
        einfo->sdltexture_noalpha = NULL;
    }
    #endif
    return 1;
}

S3DEXP int spew3d_texture_GetSize(
        s3d_texture_t tid, int32_t *out_width,
        int32_t *out_height
        ) {
    mutex_Lock(_texlist_mutex);
    if (!_internal_spew3d_ForceLoadTexture(tid)) {
        mutex_Release(_texlist_mutex);
        return 0;
    }
    spew3d_texture_extrainfo *extrainfo = (
        spew3d_extrainfo(tid)
    );
    *out_width = extrainfo->width;
    *out_height = extrainfo->height;
    mutex_Release(_texlist_mutex);
    return 1;
}

static int _unregister_texid_from_hashmap(
        const char *id, int wascorrespondstofile
        ) {
    assert(mutex_IsLocked(_texlist_mutex));
    int unregistercount = 0;
    uint16_t hash = spew3d_simplehash(id);
    spew3d_texlist_idhashmap_bucket *bucket =
        _internal_spew3d_texlist_hashmap[hash %
        SPEW3D_TEXLIST_IDHASHMAP_SIZE];
    spew3d_texlist_idhashmap_bucket *parentbucket = NULL;
    while (bucket != NULL) {
        const uint64_t idx = bucket->texlist_slot_idx;
        assert(id >= 0 && idx < _internal_spew3d_texlist_count);
        if (_internal_spew3d_texlist[idx].idstring != NULL &&
                _internal_spew3d_texlist[idx].correspondstofile ==
                (wascorrespondstofile != 0) &&
                strcmp(_internal_spew3d_texlist[idx].idstring,
                    id) == 0) {
            unregistercount++;
            spew3d_texlist_idhashmap_bucket *freebucket = bucket;
            if (parentbucket)
                parentbucket->next = bucket->next;
            else
                _internal_spew3d_texlist_hashmap[hash %
                    SPEW3D_TEXLIST_IDHASHMAP_SIZE] = bucket->next;
            bucket = bucket->next;
            continue;
        }
        parentbucket = bucket;
        bucket = bucket->next;
    }
    return unregistercount;
}

S3DHID s3d_texture_t _internal_spew3d_texture_NewEx(
        const char *name, const char *path, int vfsflags,
        int fromfile
        ) {
    #if defined(DEBUG_SPEW3D_TEXTURE)
    fprintf(stderr,
        "spew3d_texture.c: debug: "
        "_internal_spew3d_texture_NewEx called -> "
        "(\"%s\", \"%s\", %d, %d)\n",
        name, path, vfsflags, fromfile);
    #endif
    char *normpath = (
        fromfile ? spew3d_vfs_NormalizePath(path) : NULL
    );
    if (!normpath) {
        return 0;
    }
    uint32_t idlen = (
        fromfile ? strlen(normpath) + 2 : strlen(name) + 2
    );
    char *id = malloc(idlen + 1);
    if (!id) {
        free(normpath);
        return 0;
    }
    memcpy(id + 2, (fromfile ? normpath : name),
        (fromfile ? strlen(normpath) + 1 : strlen(name) + 1));
    free(normpath);
    normpath = NULL;

    int loadfromvfs = 0;
    int _innerexistsresult = 0;
    if (fromfile &&
            (vfsflags & VFSFLAG_NO_VIRTUALPAK_ACCESS) == 0 &&
            ((vfsflags & VFSFLAG_NO_REALDISK_ACCESS) != 0 ||
             (spew3d_vfs_Exists(path,
                  VFSFLAG_NO_REALDISK_ACCESS, &_innerexistsresult,
                  NULL) != 0 && _innerexistsresult))) {
        loadfromvfs = 1;
        id[0] = 'v';
        id[1] = ':';
        vfsflags |= VFSFLAG_NO_REALDISK_ACCESS;
    } else {
        id[0] = 'd';
        id[1] = ':';
        vfsflags |= VFSFLAG_NO_VIRTUALPAK_ACCESS;
    }
    #if defined(DEBUG_SPEW3D_TEXTURE)
    fprintf(stderr,
        "spew3d_texture.c: debug: "
        "_internal_spew3d_texture_NewEx id:\"%s\", "
        "path:\"%s\", name: \"%s\", vfsflags:%d\n",
        id, path, name, vfsflags);
    #endif

    mutex_Lock(_texlist_mutex);

    if (idlen >= _internal_tex_get_buf_size) {
        uint32_t newsize = (
            idlen + 20
        );
        char *_internal_tex_get_buf_new = malloc(
            newsize
        );
        if (!_internal_tex_get_buf_new) {
            free(id);
            mutex_Release(_texlist_mutex);
            return 0;
        }
        _internal_tex_get_buf =
            _internal_tex_get_buf_new;
        _internal_tex_get_buf_size = newsize;
    }
    assert(idlen >= 2);
    assert(id[idlen] == '\0');
    memcpy(_internal_tex_get_buf, id + 2, (idlen - 2) + 1);
    free(id);
    id = NULL;
    if (strlen(_internal_tex_get_buf) == 0) {
        mutex_Release(_texlist_mutex);
        return 0;
    }

    // Check if this texture is already in the global hashmap:
    _internal_spew3d_ensure_texhash();
    assert(_internal_spew3d_texlist_hashmap != NULL);
    uint16_t idhash = spew3d_simplehash(_internal_tex_get_buf);
    spew3d_texlist_idhashmap_bucket *bucket =
        _internal_spew3d_texlist_hashmap[idhash %
        SPEW3D_TEXLIST_IDHASHMAP_SIZE];
    while (bucket != NULL) {
        const uint64_t idx = bucket->texlist_slot_idx;
        assert(id >= 0 && idx < _internal_spew3d_texlist_count);
        if (_internal_spew3d_texlist[idx].idstring != NULL &&
                _internal_spew3d_texlist[idx].correspondstofile ==
                (fromfile != 0) &&
                strcmp(_internal_spew3d_texlist[idx].idstring,
                   _internal_tex_get_buf) == 0) {
            mutex_Release(_texlist_mutex);
            return idx + 1;
        }
        assert(bucket != bucket->next);
        bucket = bucket->next;
    }

    // If we arrive here, the texture doesn't exist yet.

    #ifndef NDEBUG
    // Sanity check:
    if (_spew3d_check_texidstring_used(_internal_tex_get_buf)) {
        mutex_Release(_texlist_mutex);
        fprintf(stderr, "spew3d_texture.c: error: critical "
            "programming error by application, name clash "
            "between a writable texture and another different "
            "writable or non-writable texture (which is "
            "not allowed");
        assert(!_spew3d_check_texidstring_used(_internal_tex_get_buf));
        _exit(1);
    }
    #endif

    // Allocate new slot in hash map:
    spew3d_texlist_idhashmap_bucket *newbucket = malloc(
        sizeof(*newbucket));
    if (!newbucket) {
        mutex_Release(_texlist_mutex);
        return 0;
    }
    memset(newbucket, 0, sizeof(*newbucket));
    newbucket->next = (
        _internal_spew3d_texlist_hashmap[idhash %
        SPEW3D_TEXLIST_IDHASHMAP_SIZE]
    );
    newbucket->texlist_slot_idx = _internal_spew3d_texlist_count;

    // Allocate new slot in global list:
    int64_t newcount = _internal_spew3d_texlist_count + 1;
    s3d_texture_info *new_texlist = realloc(
        _internal_spew3d_texlist,
        sizeof(*new_texlist) * newcount
    );
    if (!new_texlist) {
        free(newbucket);
        mutex_Release(_texlist_mutex);
        return 0;
    }
    _internal_spew3d_texlist = new_texlist;

    // Allocate actual entry:
    char *iddup = strdup(_internal_tex_get_buf);
    if (!iddup) {
        free(newbucket);
        mutex_Release(_texlist_mutex);
        return 0;
    }
    char *pathdup = strdup(_internal_tex_get_buf);
    if ((fromfile || path != NULL) && !pathdup) {
        free(newbucket);
        free(iddup);
        mutex_Release(_texlist_mutex);
        return 0;
    }
    spew3d_texture_extrainfo *extrainfo = malloc(
        sizeof(*extrainfo)
    );
    if (!extrainfo) {
        free(pathdup);
        free(newbucket);
        free(iddup);
        mutex_Release(_texlist_mutex);
        return 0;
    }
    memset(extrainfo, 0, sizeof(*extrainfo));
    s3d_texture_info *newinfo = &_internal_spew3d_texlist[
        _internal_spew3d_texlist_count
    ];
    memset(newinfo, 0, sizeof(*newinfo));
    extrainfo->parent = newinfo;
    newinfo->idstring = iddup;
    newinfo->diskpath = pathdup;
    newinfo->correspondstofile = (fromfile != 0);
    newinfo->loaded = 0;
    newinfo->_internal = extrainfo;
    newinfo->vfsflags = vfsflags;

    // Place new entry in hash map and list:
    _internal_spew3d_texlist_hashmap[idhash %
        SPEW3D_TEXLIST_IDHASHMAP_SIZE] = (
            newbucket);

    _internal_spew3d_texlist_count += 1;
    assert(_spew3d_check_texidstring_used(iddup));
    uint64_t result_id = _internal_spew3d_texlist_count;
    mutex_Release(_texlist_mutex);
    return result_id;
}

S3DEXP int spew3d_texture_Draw(
        s3d_window *win,
        s3d_texture_t tid,
        s3d_point point, int centered, s3dnum_t scale, s3dnum_t angle,
        s3dnum_t tint_red, s3dnum_t tint_green, s3dnum_t tint_blue,
        s3dnum_t transparency,
        int withalphachannel
        ) {
    int32_t x, y;
    spew3d_window_PointToCanvasDrawPixels(
        win, point, &x, &y
    );
    return spew3d_texture_DrawAtCanvasPixels(
        win, tid, x, y, centered, scale, angle,
        tint_red, tint_green, tint_blue, transparency,
        withalphachannel
    );
}

S3DEXP int spew3d_texture_DrawAtCanvasPixels(
        s3d_window *win,
        s3d_texture_t tid,
        int32_t x, int32_t y, int centered,
        s3dnum_t scale, s3dnum_t angle,
        s3dnum_t tint_red, s3dnum_t tint_green, s3dnum_t tint_blue,
        s3dnum_t transparency,
        int withalphachannel
        ) {
    mutex_Lock(_texlist_mutex);
    s3d_event e = {0};
    e.kind = S3DEV_INTERNAL_CMD_SPRITEDRAW;
    e.spritedraw.win_id = spew3d_window_GetID(win);
    e.spritedraw.tid = tid;
    e.spritedraw.pixel_x = x;
    e.spritedraw.pixel_y = y;
    e.spritedraw.centered = centered;
    e.spritedraw.scale = scale;
    e.spritedraw.angle = angle;
    e.spritedraw.tint_red = tint_red;
    e.spritedraw.tint_green = tint_green;
    e.spritedraw.tint_blue = tint_blue;
    e.spritedraw.transparency = transparency;
    e.spritedraw.withalphachannel = withalphachannel;
    mutex_Release(_texlist_mutex);
    return spew3d_event_q_Insert(_spew3d_event_GetInternalQueue(), &e);
}

#ifndef SPEW3D_OPTION_DISABLE_SDL
S3DHID SDL_Texture *_internal_spew3d_MainThreadOnly_GetTex_nolock(
        s3d_window *win, s3d_texture_t tex, int withalphachannel
        ) {
    assert(mutex_IsLocked(_texlist_mutex));
    s3d_texture_info *tinfo = _internal_spew3d_texinfo_nolock(tex);
    spew3d_texture_extrainfo *extrainfo = (
        spew3d_extrainfo(tex)
    );
    if (!_internal_spew3d_ForceLoadTexture(tex))
        return NULL;
    SDL_Texture *sdltex = NULL;
    int gpuupload = _internal_spew3d_TextureToGPU(
        win, tex, withalphachannel, &sdltex
    );
    if (gpuupload == 0) {
        tinfo->loadingfailed = 1;
        #if defined(DEBUG_SPEW3D_TEXTURE)
        fprintf(stderr,
            "spew3d_texture.c: debug: "
            "spew3d_texture_DrawToCanvas(): "
            "failed to access, decode, or "
            "do GPU upload of texture\n");
        #endif
        return NULL;
    }
    return sdltex;
}
#endif

S3DHID int _spew3d_texture_ProcessSpriteDrawReq(s3d_event *e) {
    assert(mutex_IsLocked(_texlist_mutex));

    s3d_window *win = spew3d_window_GetByID(e->spritedraw.win_id);
    s3d_texture_t tid = e->spritedraw.win_id;
    int32_t x = e->spritedraw.pixel_x;
    int32_t y = e->spritedraw.pixel_y;
    int centered = e->spritedraw.centered;
    s3dnum_t scale = e->spritedraw.scale;
    s3dnum_t angle = e->spritedraw.angle;
    s3dnum_t tint_red = e->spritedraw.tint_red;
    s3dnum_t tint_green = e->spritedraw.tint_green;
    s3dnum_t tint_blue = e->spritedraw.tint_blue;
    s3dnum_t transparency = e->spritedraw.transparency;
    int withalphachannel = e->spritedraw.withalphachannel;

    s3d_texture_info *tinfo = _internal_spew3d_texinfo_nolock(tid);
    spew3d_texture_extrainfo *extrainfo = (
        spew3d_extrainfo(tid)
    );
    if (!_internal_spew3d_ForceLoadTexture(tid))
        return 1;

    if (
            #ifndef SPEW3D_OPTION_DISABLE_SDL
            extrainfo->forcenosdl
            #else
            1
            #endif
            ) {
        if (transparency < (1.0 / 256.0) * 0.5)
            return 1;

        // FIXME: implement this, the no SDL2 render path.
        return 1;
    }
    #ifndef SPEW3D_OPTION_DISABLE_SDL
    SDL_Renderer *renderer = NULL;
    spew3d_window_GetSDLWindowAndRenderer(
        win, NULL, &renderer
    );
    SDL_Texture *tex = _internal_spew3d_MainThreadOnly_GetTex_nolock(
        win, tid, withalphachannel
    );
    if (!tex) {
        return 1;
    }

    double transparency_dbl = transparency;
    if (transparency_dbl < (1.0 / 256.0) * 0.5) {
        return 1;
    }

    uint8_t old_r, old_g, old_b, old_a;
    if (SDL_GetRenderDrawColor(renderer,
            &old_r, &old_g, &old_b, &old_a) != 0) {
        return 1;
    }
    uint8_t draw_r = fmax(0, fmin(255, (double)tint_red * 256.0));
    uint8_t draw_g = fmax(0, fmin(255, (double)tint_green * 255.0));
    uint8_t draw_b = fmax(0, fmin(255, (double)tint_blue * 255.0));
    uint8_t draw_a = fmax(0, fmin(255, transparency_dbl * 255.0));
    if (draw_a <= 0)
        return 1;
    if (SDL_SetRenderDrawColor(renderer,
            draw_r, draw_g, draw_b, draw_a) != 0 ||
            SDL_SetRenderDrawBlendMode(renderer,
            SDL_BLENDMODE_BLEND) != 0) {
        return 1;
    }
    SDL_Rect r = {0};
    r.x = x - (centered ?
        ((int32_t)(extrainfo->width * scale) / 2) : 0);
    r.y = y - (centered ?
        ((int32_t)(extrainfo->height * scale) / 2) : 0);
    r.w = round(extrainfo->width * scale);
    r.h = round(extrainfo->height * scale);
    SDL_SetRenderDrawBlendMode(renderer,
        SDL_BLENDMODE_BLEND);
    if (SDL_RenderCopyEx(renderer, tex, NULL, &r,
            -angle, NULL, SDL_FLIP_NONE) != 0)
        return 1;
    if (SDL_SetRenderDrawColor(renderer,
            old_r, old_g, old_b, old_a) != 0)
        return 1;
    return 1;
    #endif  // #ifndef SPEW3D_OPTION_DISABLE_SDL
}

S3DEXP s3d_texture_t spew3d_texture_FromFile(
        const char *path, int vfsflags
        ) {
    return _internal_spew3d_texture_NewEx(
        NULL, path, vfsflags, 1
    );
}

S3DEXP s3d_texture_t spew3d_texture_NewWritable(
        const char *name, uint32_t w, uint32_t h
        ) {
    s3d_texture_t tex = (
        _internal_spew3d_texture_NewEx(name, NULL, 0, 0)
    );
    if (tex == 0)
        return 0;
    mutex_Lock(_texlist_mutex);
    s3d_texture_info *tinfo = (
        _internal_spew3d_texinfo_nolock(tex)
    );
    spew3d_texture_extrainfo *extrainfo = (
        spew3d_extrainfo(tex)
    );
    assert(tinfo != NULL); 
    assert(!tinfo->correspondstofile);
    assert(tinfo->idstring != NULL);
    if (!tinfo->loaded) {
        assert(tex == _internal_spew3d_texlist_count - 1);
        extrainfo->width = w;
        extrainfo->height = h;
        int64_t pixelcount = ((int64_t)w) * ((int64_t)h);
        if (pixelcount <= 0) pixelcount = 1;
        extrainfo->pixels = malloc(4 * pixelcount);
        if (!extrainfo->pixels) {
            int uregcount = (
                _unregister_texid_from_hashmap(tinfo->idstring, 0)
            );
            assert(uregcount == 1);
            free(tinfo->_internal);
            free(tinfo->idstring);
            free(tinfo->diskpath);
            _internal_spew3d_texlist_count--;
            mutex_Release(_texlist_mutex);
            return 0;
        }
        memset(extrainfo->pixels, 0, 4 * pixelcount);
        tinfo->loaded = 1;
    }
    mutex_Release(_texlist_mutex);
    return tex;
}

S3DEXP void spew3d_texture_Destroy(s3d_texture_t tid) {
    mutex_Lock(_texlist_mutex);
    s3d_event e = {0};
    e.kind = S3DEV_INTERNAL_CMD_TEXDELETE;
    e.texdelete.tid = tid;
    mutex_Release(_texlist_mutex);
    _spew3d_event_q_InsertForce(_spew3d_event_GetInternalQueue(), &e);
}

S3DHID int _spew3d_texture_ProcessTexDestroyReq(s3d_event *ev) {
    assert(mutex_IsLocked(_texlist_mutex));
    s3d_texture_t tid = ev->texdelete.tid;
    assert(tid >= 0 && tid <= _internal_spew3d_texlist_count);
    if (tid == 0) {
        return 1;
    }
    s3d_texture_info *tinfo = _internal_spew3d_texinfo_nolock(tid);
    assert(tinfo != NULL);
    assert(!tinfo->correspondstofile);
    assert(tinfo->idstring != NULL);
    if (!tinfo->loaded) {
        return 1;
    }
    int uregcount = (
        _unregister_texid_from_hashmap(tinfo->idstring, 0)
    );
    assert(uregcount == 1);
    spew3d_texture_extrainfo *extrainfo = (
        spew3d_extrainfo(tid)
    );
    if (extrainfo) {
        assert(extrainfo->loadingjob == NULL);
        free(extrainfo->pixels);
        #ifndef SPEW3D_OPTION_DISABLE_SDL
        if (extrainfo->sdltexture_alpha)
            SDL_DestroyTexture(extrainfo->sdltexture_alpha);
        if (extrainfo->sdltexture_noalpha)
            SDL_DestroyTexture(extrainfo->sdltexture_noalpha);
        #endif  // #ifndef SPEW3D_OPTION_DISABLE_SDL
        free(extrainfo);
    }
    free(tinfo->idstring);
    free(tinfo->diskpath);
    tinfo->idstring = NULL;
    tinfo->diskpath = NULL;
    tinfo->_internal = NULL;
    tinfo->loaded = 0;
    return 1;
}

S3DEXP s3d_texture_t spew3d_texture_NewWritableFromFile(
        const char *name,
        const char *original_path,
        int original_vfsflags
        ) {
    return (
        _internal_spew3d_texture_NewEx(
            name, original_path, original_vfsflags, 0));
}

S3DEXP int spew3d_texture_InternalMainThreadProcessEvent(
        s3d_event *e
        ) {
    thread_MarkAsMainThread();

    s3d_equeue *eq = _spew3d_event_GetInternalQueue();
    assert(eq != NULL);

    mutex_Lock(_texlist_mutex);
    if (e->kind == S3DEV_INTERNAL_CMD_TEXTURELOCK_LOCKPIXELSTOFINISH) {
        if (!_spew3d_window_ProcessTexLockPixelsReq(e)) {
            mutex_Release(_texlist_mutex);
            _spew3d_event_q_InsertForce(eq, e);
        }
        mutex_Release(_texlist_mutex);
        return 1;
    } else if (e->kind == S3DEV_INTERNAL_CMD_SPRITEDRAW) {
        if (!_spew3d_texture_ProcessSpriteDrawReq(e)) {
            mutex_Release(_texlist_mutex);
            _spew3d_event_q_InsertForce(eq, e);
        }
        mutex_Release(_texlist_mutex);
        return 1;
    } else if (e->kind == S3DEV_INTERNAL_CMD_TEXDELETE) {
        if (!_spew3d_texture_ProcessTexDestroyReq(e)) {
            mutex_Release(_texlist_mutex);
            _spew3d_event_q_InsertForce(eq, e);
        }
        mutex_Release(_texlist_mutex);
        return 1;
    }
    mutex_Release(_texlist_mutex);
    return 0;
}

#endif  // SPEW3D_IMPLEMENTATION

