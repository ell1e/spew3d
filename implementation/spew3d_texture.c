/* Spew3D is Copyright 2022 ell1e et al.

Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or
sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following
conditions:

The above copyright notice and this permission notice shall
be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY
KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS
OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifdef SPEW3D_IMPLEMENTATION

#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <SDL2/SDL.h>
#include <unistd.h>

static inline uint16_t spew3d_simplehash(const char *k);

// Some global variables:
uint64_t _internal_spew3d_texlist_count;
spew3d_texture_info *_internal_spew3d_texlist;
extern SDL_Window *_internal_spew3d_outputwindow;
extern SDL_Renderer *_internal_spew3d_outputrenderer;

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
    spew3d_texture_info *parent;
    SDL_Texture *sdltexture_alpha, *sdltexture_noalpha;
} spew3d_texture_extrainfo;


static void __attribute__((constructor)) _internal_spew3d_ensure_texhash() {
    if (_internal_spew3d_texlist_hashmap != NULL)
        return;

    _internal_spew3d_texlist_hashmap = malloc(
        sizeof(*_internal_spew3d_texlist_hashmap) *
        SPEW3D_TEXLIST_IDHASHMAP_SIZE
    );
    if (!_internal_spew3d_texlist_hashmap) {
        fprintf(stderr, "spew3d_texture.c: error: "
            "failed to allocate _internal_spew3d_texlist_hashmap");
        _exit(1);
    }
    memset(_internal_spew3d_texlist_hashmap, 0,
        sizeof(*_internal_spew3d_texlist_hashmap) *
        SPEW3D_TEXLIST_IDHASHMAP_SIZE
    );
}


static inline spew3d_texture_info *_fast_spew3d_texinfo(
        spew3d_texture_t id
        ) {
    assert(id > 0 && id <= _internal_spew3d_texlist_count);
    return &_internal_spew3d_texlist[id - 1];
}


spew3d_texture_info *spew3d_texinfo(
        spew3d_texture_t id
        ) {
    assert(id > 0 && id <= _internal_spew3d_texlist_count);
    return &_internal_spew3d_texlist[id - 1];
}


static int _spew3d_check_texidstring_used(
        const char *id) {
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


static int _internal_spew3d_ForceLoadTexture(spew3d_texture_t tid) {
    spew3d_texture_info *tinfo = _fast_spew3d_texinfo(tid);
    assert(tinfo != NULL && tinfo->idstring != NULL);
    if (tinfo->loaded || (!tinfo->correspondstofile &&
            !tinfo->diskpath))
        return 1;
    if (tinfo->loadingfailed)
        return 0;

    assert(!tinfo->loaded);
    assert(tinfo->diskpath != NULL);
    char *imgcompressed = NULL;
    uint64_t imgcompressedlen = 0;
    if (!spew3d_vfs_FileToBytes(
            tinfo->diskpath, &imgcompressed,
            &imgcompressedlen)) {
        #if defined(DEBUG_SPEW3D_TEXTURE)
        fprintf(stderr,
            "spew3d_texture.c: debug: "
            "_internal_spew3d_texture_ForceLoadTexture "
            "failed to read disk data for "
            "texture: \"%s\"\n",
            tinfo->diskpath);
        #endif
        tinfo->loadingfailed = 1;
        return 0;
    }
    #if defined(DEBUG_SPEW3D_TEXTURE)
    fprintf(stderr,
        "spew3d_texture.c: debug: "
        "_internal_spew3d_texture_ForceLoadTexture "
        "decoding this texture: %s\n",
        tinfo->diskpath);
    #endif

    int w = 0;
    int h = 0;
    int n = 0;
    unsigned char *data32 = stbi_load_from_memory(
        (unsigned char *)imgcompressed,
        imgcompressedlen, &w, &h, &n, 4
    );
    if (!data32) {
        #if defined(DEBUG_SPEW3D_TEXTURE)
        fprintf(stderr,
            "spew3d_texture.c: debug: "
            "_internal_spew3d_texture_ForceLoadTexture "
            "failed to decode or allocate image "
            "for texture: \"%s\"\n",
            tinfo->diskpath);
        #endif
        tinfo->loadingfailed = 1;
        return 0;
    }
    tinfo->width = w;
    tinfo->height = h;
    tinfo->pixels = data32;
    tinfo->loaded = 1;
    #if defined(DEBUG_SPEW3D_TEXTURE)
    fprintf(stderr,
        "spew3d_texture.c: debug: "
        "_internal_spew3d_texture_ForceLoadTexture "
        "suceeded for texture: \"%s\"\n",
        tinfo->diskpath);
    #endif
    return 1;
}


static int _internal_spew3d_TextureToGPU(
        spew3d_texture_t tid, int alpha,
        SDL_Texture **out_tex
        ) {
    if (!_internal_spew3d_ForceLoadTexture(tid))
        return 0;
    spew3d_texture_info *tinfo = _fast_spew3d_texinfo(tid);
    if (tinfo->loadingfailed)
        return 0;
    assert(tinfo != NULL && tinfo->idstring != NULL);
    assert(tinfo->loaded && tinfo->pixels != NULL);
    spew3d_texture_extrainfo *extrainfo = (
        (spew3d_texture_extrainfo *)tinfo->_internal
    );
    if (alpha && extrainfo->sdltexture_alpha) {
        *out_tex = extrainfo->sdltexture_alpha;
        return 1;
    } else if (!alpha && extrainfo->sdltexture_noalpha) {
        *out_tex = extrainfo->sdltexture_noalpha;
        return 1;
    }

    SDL_Renderer *renderer = (
        _internal_spew3d_outputrenderer
    ); 
    SDL_Surface *s = SDL_CreateRGBSurfaceFrom(
        tinfo->pixels, tinfo->width, tinfo->height,
        32, tinfo->width * 4, 0x000000ff,
        0x0000ff00, 0x00ff0000,
        (alpha ? 0xff000000 : 0));
    if (!s)
        return 0;
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


static int _unregister_texid_from_hashmap(
        const char *id, int wascorrespondstofile
        ) {
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


void _internal_normpath(char *p) {
    uint32_t plen = strlen(p);
    uint32_t i = 0;
    while (i < plen) {
        if (p[i] == '\\') {
            p[i] = '/';
        }
        if (p[i] == '/' && i > 0 && p[i - 1] == '/') {
            if (i + 1 < plen)
                memcpy(&p[i], &p[i + 1],
                    (plen - i - 1));  // Ignores null terminator!
            plen -= 1;
            p[plen] = '\0';  // Re-add null terminator.
            // Don't do i++!
            continue; 
        } else if (p[i] == '/' && i > 0 && p[i - 1] == '.' &&
                (i <= 1 || p[i - 2] == '/')) {
            if (i + 1 < plen)
                memcpy(&p[i - 1], &p[i + 1],
                    (plen - i));  // Ignores null terminator!
            plen -= 2;
            p[plen] = '\0';  // Re-add null terminator.
            i--;  // Intentional.
            continue;
        } else if (p[i] == '/' && i + 1 >= plen) {
            plen--;
            p[plen] = '\0';
            break;
        }
        i++;
    }
}


char *_internal_tex_get_buf = NULL;
uint32_t _internal_tex_get_buf_size = 0;


spew3d_texture_t _internal_spew3d_texture_NewEx(
        const char *name, const char *path, int fromfile
        ) {
    uint32_t idlen = (fromfile ? strlen(path) : strlen(name));
    const char *id = (fromfile ? path : name);
    if (idlen >= _internal_tex_get_buf_size) {
        uint32_t newsize = (
            idlen + 20
        );
        char *_internal_tex_get_buf_new = malloc(
            newsize);
        if (!_internal_tex_get_buf_new)
            return 0;
        _internal_tex_get_buf =
            _internal_tex_get_buf_new;
        _internal_tex_get_buf_size = newsize;
    }
    memcpy(_internal_tex_get_buf, id, idlen + 1);
    if (fromfile) {
        _internal_normpath(_internal_tex_get_buf);
        #if defined(_WIN32) || defined(_WIN64)
        if (strlen(_internal_tex_get_buf) >= 2 &&
                _internal_tex_get_buf[1] == ':')
            return 0;
        #endif
        if (strlen(_internal_tex_get_buf) >= 1 &&
                _internal_tex_get_buf[0] == '/')
            return 0;
        if (strlen(_internal_tex_get_buf) >= 2 &&
                _internal_tex_get_buf[0] == '.' &&
                _internal_tex_get_buf[1] == '.' && (
                strlen(_internal_tex_get_buf) == 2 ||
                _internal_tex_get_buf[1] == '/'))
            return 0;
    }
    if (strlen(_internal_tex_get_buf) == 0)
        return 0;

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
            return idx + 1;
        }
        assert(bucket != bucket->next);
        bucket = bucket->next;
    }

    // If we arrive here, the texture doesn't exist yet.

    #ifndef NDEBUG
    // Sanity check:
    if (_spew3d_check_texidstring_used(_internal_tex_get_buf)) {
        fprintf(stderr, "spew3d_texture.c: error: critical "
            "programming error by application, name clash "
            "between a writable texture and another "
            "writable or non-writable texture (which is "
            "not allowed");
        assert(!_spew3d_check_texidstring_used(_internal_tex_get_buf));
        _exit(1);
    }
    #endif

    // Allocate new slot in hash map:
    spew3d_texlist_idhashmap_bucket *newbucket = malloc(
        sizeof(*newbucket));
    if (!newbucket)
        return 0;
    memset(newbucket, 0, sizeof(*newbucket));
    newbucket->next = (
        _internal_spew3d_texlist_hashmap[idhash %
        SPEW3D_TEXLIST_IDHASHMAP_SIZE]
    );
    newbucket->texlist_slot_idx = _internal_spew3d_texlist_count;

    // Allocate new slot in global list:
    int64_t newcount = _internal_spew3d_texlist_count + 1;
    spew3d_texture_info *new_texlist = realloc(
        _internal_spew3d_texlist,
        sizeof(*new_texlist) * newcount
    );
    if (!new_texlist) {
        free(newbucket);
        return 0;
    }
    _internal_spew3d_texlist = new_texlist;

    // Allocate actual entry:
    char *iddup = strdup(_internal_tex_get_buf);
    if (!iddup) {
        free(newbucket);
        return 0;
    }
    char *pathdup = (fromfile ? strdup(_internal_tex_get_buf) :
        (path != NULL ? strdup(path) : NULL));
    if ((fromfile || path != NULL) && !pathdup) {
        free(newbucket);
        free(iddup);
        return 0;
    }
    spew3d_texture_extrainfo *extrainfo = malloc(
        sizeof(*extrainfo)
    );
    if (!extrainfo) {
        free(pathdup);
        free(newbucket);
        free(iddup);
        return 0;
    }
    memset(extrainfo, 0, sizeof(*extrainfo));
    spew3d_texture_info *newinfo = &_internal_spew3d_texlist[
        _internal_spew3d_texlist_count
    ];
    memset(newinfo, 0, sizeof(*newinfo));
    extrainfo->parent = newinfo;
    newinfo->idstring = iddup;
    newinfo->diskpath = pathdup;
    newinfo->correspondstofile = (fromfile != 0);
    newinfo->loaded = 0;
    newinfo->_internal = extrainfo;

    // Place new entry in hash map and list:
    _internal_spew3d_texlist_hashmap[idhash %
        SPEW3D_TEXLIST_IDHASHMAP_SIZE] = (
            newbucket);

    _internal_spew3d_texlist_count += 1;
    assert(_spew3d_check_texidstring_used(iddup));
    return _internal_spew3d_texlist_count;
}


int spew3d_texture_Draw(
        spew3d_texture_t tid,
        int32_t x, int32_t y, double scale, double angle,
        double tint_red, double tint_green, double tint_blue,
        double transparency,
        int withalphachannel
        ) {
    spew3d_texture_info *tinfo = _fast_spew3d_texinfo(tid);
    assert(_internal_spew3d_outputwindow != NULL);
    assert(_internal_spew3d_outputrenderer != NULL);

    SDL_Renderer *renderer = _internal_spew3d_outputrenderer;
    SDL_Texture *tex = NULL;
    if (tinfo->loadingfailed)
        return 0;
    if (!_internal_spew3d_TextureToGPU(
            tid, withalphachannel, &tex
            )) {
        tinfo->loadingfailed = 1;
        #if defined(DEBUG_SPEW3D_TEXTURE)
        fprintf(stderr,
            "spew3d_texture.c: debug: "
            "spew3d_texture_Draw "
            "failed to load, decode, or "
            "GPU upload texture\n");
        #endif
        return 0;
    }

    if (transparency < (1.0 / 256.0) * 0.5)
        return 0;

    uint8_t old_r, old_g, old_b, old_a;
    if (!SDL_GetRenderDrawColor(renderer,
            &old_r, &old_g, &old_b, &old_a))
        return 0;
    uint8_t draw_r = fmax(0, fmin(255, tint_red * 256.0));
    uint8_t draw_g = fmax(0, fmin(255, tint_green * 255.0));
    uint8_t draw_b = fmax(0, fmin(255, tint_blue * 255.0));
    uint8_t draw_a = fmax(0, fmin(255, transparency * 255.0));
    if (draw_a <= 0)
        return 0;
    if (!SDL_SetRenderDrawColor(renderer,
            draw_r, draw_g, draw_b, draw_a) ||
            SDL_SetRenderDrawBlendMode(renderer,
            SDL_BLENDMODE_BLEND)) {
        return 0;
    }
    SDL_Rect r = {0};
    r.x = x;
    r.y = y;
    r.w = tinfo->width;
    r.h = tinfo->height;
    if (!SDL_RenderCopyEx(renderer, tex, NULL, &r,
            angle, NULL, SDL_FLIP_NONE))
        return 0;
    if (!SDL_SetRenderDrawColor(renderer,
            old_r, old_g, old_b, old_a))
        return 0;
    return 1;
}


spew3d_texture_t spew3d_texture_FromFile(
        const char *path
        ) {
    return _internal_spew3d_texture_NewEx(
        NULL, path, 1
    );
}


spew3d_texture_t spew3d_texture_NewWritable(
        const char *name, uint32_t w, uint32_t h
        ) {
    spew3d_texture_t tex = (
        _internal_spew3d_texture_NewEx(name, NULL, 0)
    );
    if (tex == 0)
        return 0;
    spew3d_texture_info *tinfo = (
        _fast_spew3d_texinfo(tex)
    );
    assert(tinfo != NULL); 
    assert(!tinfo->correspondstofile);
    assert(tinfo->idstring != NULL);
    if (!tinfo->loaded) {
        assert(tex == _internal_spew3d_texlist_count - 1);
        tinfo->width = w;
        tinfo->height = h;
        int64_t pixelcount = ((int64_t)w) * ((int64_t)h);
        if (pixelcount <= 0) pixelcount = 1;
        tinfo->pixels = malloc(4 * pixelcount);
        if (!tinfo->pixels) {
            int uregcount = (
                _unregister_texid_from_hashmap(tinfo->idstring, 0)
            );
            assert(uregcount == 1);
            free(tinfo->idstring);
            free(tinfo->diskpath);
            _internal_spew3d_texlist_count--;
            return 0;
        }
        memset(tinfo->pixels, 0, 4 * pixelcount);
        tinfo->loaded = 1;
    }
    return tex;
}


void spew3d_texture_Destroy(spew3d_texture_t tid) {
    assert(tid >= 0 && tid < _internal_spew3d_texlist_count);
    if (tid == 0)
        return;
    spew3d_texture_info *tinfo = _fast_spew3d_texinfo(tid);
    assert(tinfo != NULL);
    assert(!tinfo->correspondstofile);
    assert(tinfo->idstring != NULL);
    if (!tinfo->loaded)
        return;
    int uregcount = (
        _unregister_texid_from_hashmap(tinfo->idstring, 0)
    );
    assert(uregcount == 1);
    spew3d_texture_extrainfo *extrainfo = (
        (spew3d_texture_extrainfo *)tinfo->_internal
    );
    if (extrainfo) {
        if (extrainfo->sdltexture_alpha)
            SDL_DestroyTexture(extrainfo->sdltexture_alpha);
        if (extrainfo->sdltexture_noalpha)
            SDL_DestroyTexture(extrainfo->sdltexture_noalpha);
    }
    free(tinfo->pixels);
    free(tinfo->idstring);
    free(tinfo->diskpath);
    tinfo->idstring = NULL;
    tinfo->diskpath = NULL;
    tinfo->pixels = NULL;
    tinfo->loaded = 0;
}


spew3d_texture_t spew3d_texture_NewWritableFromFile(
        const char *name, const char *original_path
        ) {
    return (
        _internal_spew3d_texture_NewEx(
            name, original_path, 0));
}

#endif  // SPEW3D_IMPLEMENTATION

