
#ifndef SPEW3D_TEXTURE_H_
#define SPEW3D_TEXTURE_H_

#include <stdint.h>
#include <SDL2/SDL.h>

typedef int64_t spew3d_texture_t;

typedef struct spew3d_texture_info {
    char *nameorfilepath;
    uint8_t loaded, fromfile;
    uint32_t w, h;
    char *pixels;
} spew3d_texture_info;

extern int32_t _internal_spew3d_texlist_count;
extern spew3d_texture_info *_internal_spew3d_texlist;

spew3d_texture_t spew3d_texture_ByFile(const char *path);

static inline spew3d_texture_info *spew3d_texinfo(
        spew3d_texture_t id
        ) {
    if (id <= 0)
        return NULL;
    return &_internal_spew3d_texlist[id - 1];
}

spew3d_texture_t spew3d_texture_ByName(
    const char *name, uint32_t w, uint32_t h
);

void spew3d_texture_Destroy(spew3d_texture_t tid);

#endif  // SPEW3D_TEXTURE_H_

