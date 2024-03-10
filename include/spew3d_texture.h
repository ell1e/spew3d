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

#ifndef SPEW3D_TEXTURE_H_
#define SPEW3D_TEXTURE_H_

#include <stdint.h>
#if !defined(SPEW3D_OPTION_DISABLE_SDL) &&\
        !defined(SPEW3D_OPTION_DISABLE_SDL_HEADER)
#include <SDL2/SDL.h>
#endif

typedef uint64_t s3d_texture_t;
typedef struct s3d_window s3d_window;
typedef struct s3d_event s3d_event;

typedef struct s3d_texture_info {
    char *idstring, *diskpath;
    int vfsflags;
    uint8_t loaded, loadingfailed;
    uint8_t correspondstofile;

    void *_internal;
} s3d_texture_info;

S3DEXP s3d_texture_t spew3d_texture_FromFile(
    const char *path, int vfsflags
);

S3DEXP void spew3d_texinfo(
    s3d_texture_t id, s3d_texture_info *write_to
);

S3DEXP s3d_texture_t spew3d_texture_NewWritable(
    const char *name, uint32_t w, uint32_t h
);

S3DEXP s3d_texture_t spew3d_texture_NewWritableFromFile(
    const char *name, const char *original_path,
    int original_vfsflags
);

S3DEXP void spew3d_texture_Destroy(s3d_texture_t tid);

S3DEXP int spew3d_texture_Draw(
    s3d_window *win,
    s3d_texture_t tid,
    s3d_point point, int centered, s3dnum_t scale, s3dnum_t angle,
    s3dnum_t tint_red, s3dnum_t tint_green, s3dnum_t tint_blue,
    s3dnum_t transparency,
    int withalphachannel
);

S3DEXP int spew3d_texture_DrawAtCanvasPixels(
    s3d_window *win,
    s3d_texture_t tid,
    int32_t x, int32_t y, int centered,
    s3dnum_t scale, s3dnum_t angle,
    s3dnum_t tint_red, s3dnum_t tint_green, s3dnum_t tint_blue,
    s3dnum_t transparency,
    int withalphachannel
);

S3DEXP int spew3d_texture_GetSize(
    s3d_texture_t tid, int32_t *out_width,
    int32_t *out_height
);

S3DEXP const char *spew3d_texture_GetReadonlyPixels(
    s3d_texture_t tid
);

S3DEXP char *spew3d_texture_UnlockPixelsToEdit(
    s3d_texture_t tid
);

S3DEXP void spew3d_texture_LockPixelsToFinishEdit(
    s3d_texture_t tid
);

S3DEXP int spew3d_texture_InternalMainThreadProcessEvent(
    s3d_event *e
);

S3DHID s3d_texture_info *_internal_spew3d_texinfo_nolock(
    s3d_texture_t id
);

#endif  // SPEW3D_TEXTURE_H_

