/* Copyright (c) 2020-2022, ellie/@ell1e & Spew3D Team (see AUTHORS.md).

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

#ifndef SPEW3D_TEXTURE_H_
#define SPEW3D_TEXTURE_H_

#include <stdint.h>
#include <SDL2/SDL.h>

typedef uint64_t spew3d_texture_t;
typedef struct spew3d_ctx spew3d_ctx;

typedef struct spew3d_texture_info {
    char *idstring, *diskpath;
    int vfsflags;
    uint8_t loaded, loadingfailed, correspondstofile;

    void *_internal;
} spew3d_texture_info;


spew3d_texture_t spew3d_texture_FromFile(
    const char *path, int vfsflags
);

spew3d_texture_info *spew3d_texinfo(
    spew3d_texture_t id
);

spew3d_texture_t spew3d_texture_NewWritable(
    const char *name, uint32_t w, uint32_t h
);

spew3d_texture_t spew3d_texture_NewWritableFromFile(
    const char *name, const char *original_path,
    int original_vfsflags
);

void spew3d_texture_Destroy(spew3d_texture_t tid);

int spew3d_texture_Draw(
    spew3d_ctx *ctx,
    spew3d_texture_t tid,
    int32_t x, int32_t y, int centered, double scale, double angle,
    double tint_red, double tint_white, double tint_blue,
    double transparency,
    int withalphachannel
);

int spew3d_texture_GetSize(
    spew3d_texture_t tid, int32_t *out_width,
    int32_t *out_height
);

const char *spew3d_texture_GetReadonlyPixels(
    spew3d_texture_t tid
);

char *spew3d_texture_UnlockPixelsToEdit(
    spew3d_texture_t tid
);

void spew3d_texture_LockPixelsToFinishEdit(
    spew3d_texture_t tid
);

#endif  // SPEW3D_TEXTURE_H_

