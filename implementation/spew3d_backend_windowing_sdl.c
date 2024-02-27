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
#ifndef SPEW3D_OPTION_DISABLE_SDL

typedef struct s3d_backend_windowing_wininfo_sdl2 {
    uint8_t dummy;
} s3d_backend_windowing_wininfo_sdl2;

S3DHID s3d_backend_windowing_wininfo *_s3d_sdl_CreateWinInfo(
        s3d_backend_windowing *backend, s3d_window *win
        ) {
    s3d_backend_windowing_wininfo_sdl2 *winfo = malloc(
        sizeof(*winfo)
    );
    if (!winfo)
        return NULL;
    memset(winfo, 0, sizeof(*winfo));
    return (s3d_backend_windowing_wininfo *)winfo;
}

S3DEXP void spew3d_window_GetSDLWindowAndRenderer(
    s3d_window *win, SDL_Window **out_sdlwindow,
    SDL_Renderer **out_sdlrenderer
);
S3DEXP void spew3d_window_GetSDLWindowAndRenderer_nolock(
    s3d_window *win, SDL_Window **out_w,
    SDL_Renderer **out_r
);

S3DHID void _s3d_sdl_WarpMouse(s3d_backend_windowing *backend,
        s3d_window *win, s3d_backend_windowing_wininfo *wininfo,
        s3dnum_t x, s3dnum_t y) {
    int wx = floor(x);
    int wy = floor(y);
    SDL_Window *sdlwin = NULL;
    spew3d_window_GetSDLWindowAndRenderer_nolock(
        win, &sdlwin, NULL
    );  // FIXME: using no lock here isn't quite right.
    if (sdlwin != NULL) {
        SDL_WarpMouseInWindow(sdlwin, wx, wy);
    }
}

S3DHID void _s3d_sdl_DestroyWinInfo(
        s3d_backend_windowing *backend, s3d_window *win,
        s3d_backend_windowing_wininfo *winfo
        ) {
    if (!winfo)
        return;
    free(winfo);
}

s3d_backend_windowing *_singleton_sdl_backend = NULL;

S3DHID __attribute__((constructor)) static void _init_sdl2_backend() {
    s3d_backend_windowing *b = malloc(sizeof(*b));
    if (b) {
        memset(b, 0, sizeof(*b));
        b->CreateWinInfo = _s3d_sdl_CreateWinInfo;
        b->WarpMouse = _s3d_sdl_WarpMouse;
        b->DestroyWinInfo = _s3d_sdl_DestroyWinInfo;
        _singleton_sdl_backend = b;
    }
    if (_singleton_sdl_backend == NULL) {
        fprintf(stderr, "spew3d_backend_windowing_sdl.c: error: "
            "Failed to create global singleton.\n");
        _exit(1);
    }
}

S3DEXP s3d_backend_windowing *spew3d_backend_windowing_GetSDL() {
    return _singleton_sdl_backend;
}

#endif  // not SPEW3D_OPTION_DISABLE_SDL
#endif  // SPEW3D_IMPLEMENTATION

