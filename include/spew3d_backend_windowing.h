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

#ifndef SPEW3D_BACKEND_WINDOWING_H_
#define SPEW3D_BACKEND_WINDOWING_H_

typedef struct s3d_window s3d_window;
typedef struct s3d_backend_windowing s3d_backend_windowing;
typedef struct s3d_backend_windowing_gputex
    s3d_backend_windowing_gputex;

typedef struct s3d_backend_windowing_wininfo
    s3d_backend_windowing_wininfo;

enum S3dBackendWindowingKind {
    S3D_BACKEND_WINDOWING_INVALID = 0,
    S3D_BACKEND_WINDOWING_SDL = 1
};

typedef struct s3d_backend_windowing {
    int kind;

    s3d_backend_windowing_wininfo *(*CreateWinInfo)(
        s3d_backend_windowing *backend, s3d_window *win
    );
    void (*DestroyWinInfo)(
        s3d_backend_windowing *backend, s3d_window *win,
        s3d_backend_windowing_wininfo *winfo
    );
    void (*WarpMouse)(s3d_backend_windowing *backend,
        s3d_window *win, s3d_backend_windowing_wininfo *wininfo,
        s3dnum_t x, s3dnum_t y
    );
    int (*CreateWinObj)(
        s3d_backend_windowing *backend, s3d_window *win,
        s3d_backend_windowing_wininfo *backend_winfo,
        uint32_t flags,
        const char *title, uint32_t width, uint32_t height
    );
    int (*WasWinObjCreated)(
        s3d_backend_windowing *backend, s3d_window *win,
        s3d_backend_windowing_wininfo *backend_winfo
    );
    void (*ResetMouseGrab)(
        s3d_backend_windowing *backend, s3d_window *win,
        s3d_backend_windowing_wininfo *backend_winfo
    );
    void (*SetMouseGrabConstrained)(
        s3d_backend_windowing *backend, s3d_window *win,
        s3d_backend_windowing_wininfo *backend_winfo
    );
    void (*SetMouseGrabInvisibleRelative)(
        s3d_backend_windowing *backend, s3d_window *win,
        s3d_backend_windowing_wininfo *backend_winfo
    );

    int supports_gpu_textures;
    s3d_backend_windowing_gputex *(*CreateGPUTexture)(
        s3d_backend_windowing *backend, s3d_window *win,
        s3d_backend_windowing_wininfo *backend_winfo,
        void *pixel_rgba_data, uint32_t w, uint32_t h,
        int set_alpha_solid
    );
    void (*DestroyGPUTexture)(
        s3d_backend_windowing *backend, s3d_window *win,
        s3d_backend_windowing_wininfo *backend_winfo,
        s3d_backend_windowing_gputex *tex
    );
    int (*DrawSpriteAtPixels)(
        s3d_backend_windowing *backend, s3d_window *win,
        s3d_backend_windowing_wininfo *backend_winfo,
        s3d_backend_windowing_gputex *tex,
        int32_t x, int32_t y, s3dnum_t scale, s3dnum_t angle,
        s3dnum_t tint_red, s3dnum_t tint_green, s3dnum_t tint_blue,
        s3dnum_t transparency, int centered,
        int withalphachannel
    );
    int (*DrawPolygonAtPixels)(
        s3d_backend_windowing *backend, s3d_window *win,
        s3d_backend_windowing_wininfo *backend_winfo,
        s3d_backend_windowing_gputex *tex,
        s3d_pos *vertices, s3d_point *tex_points,
        s3d_color *colors
    );

    void *internal;
} s3d_backend_windowing;

#endif  // SPEW3D_BACKEND_WINDOWING_H_

