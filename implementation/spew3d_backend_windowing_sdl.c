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
    SDL_Window *sdl_win;
    SDL_Renderer *sdl_renderer;
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

S3DHID void _s3d_sdl_ResetMouseGrab(
        s3d_backend_windowing *backend, s3d_window *win,
        s3d_backend_windowing_wininfo *_backend_winfo
        ) {
    s3d_backend_windowing_wininfo_sdl2 *backend_winfo =
        (s3d_backend_windowing_wininfo_sdl2 *)_backend_winfo;
    SDL_CaptureMouse(SDL_FALSE);
    SDL_SetRelativeMouseMode(SDL_FALSE);
    SDL_ShowCursor(SDL_ENABLE);
    SDL_SetWindowGrab(backend_winfo->sdl_win, SDL_FALSE);
}

S3DHID int _s3d_sdl_WasWinObjCreated(
        s3d_backend_windowing *backend, s3d_window *win,
        s3d_backend_windowing_wininfo *_backend_winfo
        ) {
    s3d_backend_windowing_wininfo_sdl2 *backend_winfo =
        (s3d_backend_windowing_wininfo_sdl2 *)_backend_winfo;
    return backend_winfo->sdl_win != NULL;
}

S3DHID void _s3d_sdl_SetMouseGrabConstrained(
        s3d_backend_windowing *backend, s3d_window *win,
        s3d_backend_windowing_wininfo *_backend_winfo
        ) {
    s3d_backend_windowing_wininfo_sdl2 *backend_winfo =
        (s3d_backend_windowing_wininfo_sdl2 *)_backend_winfo;
    SDL_SetWindowGrab(backend_winfo->sdl_win, SDL_TRUE);
    SDL_CaptureMouse(SDL_TRUE);
    SDL_ShowCursor(SDL_ENABLE);
}

S3DHID void _s3d_sdl_SetMouseGrabInvisibleRelative(
        s3d_backend_windowing *backend, s3d_window *win,
        s3d_backend_windowing_wininfo *_backend_winfo
        ) {
    s3d_backend_windowing_wininfo_sdl2 *backend_winfo =
        (s3d_backend_windowing_wininfo_sdl2 *)_backend_winfo;
    SDL_SetWindowGrab(backend_winfo->sdl_win, SDL_TRUE);
    SDL_CaptureMouse(SDL_TRUE);
    SDL_ShowCursor(SDL_DISABLE);
}

S3DHID int _s3d_sdl_CreateWinObj(
        s3d_backend_windowing *backend, s3d_window *win,
        s3d_backend_windowing_wininfo *_backend_winfo,
        uint32_t flags,
        const char *title, uint32_t width, uint32_t height
        ) {
    s3d_backend_windowing_wininfo_sdl2 *backend_winfo =
        (s3d_backend_windowing_wininfo_sdl2 *)_backend_winfo;

    int isfullscreen = (
        (flags & SPEW3D_WINDOW_FLAG_FULLSCREEN) != 0
    );

    // Try to create a 3d accelerated OpenGL window:
    SDL_Window *window = SDL_CreateWindow(
        title, SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED, width, height,
        SDL_WINDOW_RESIZABLE|
        (isfullscreen ? SDL_WINDOW_FULLSCREEN:0)|
        (((flags &
                SPEW3D_WINDOW_FLAG_FORCE_NO_3D_ACCEL) == 0) ?
            SDL_WINDOW_OPENGL:0)|
        SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Renderer *renderer = NULL;
    if (window) {  // Try to create 3d accel OpenGL renderer:
        renderer = SDL_CreateRenderer(window, -1,
            SDL_RENDERER_PRESENTVSYNC|
            (((flags &
                    SPEW3D_WINDOW_FLAG_FORCE_NO_3D_ACCEL) == 0) ?
                SDL_RENDERER_ACCELERATED:
                SDL_RENDERER_SOFTWARE));
        if (!renderer) {
            SDL_DestroyWindow(window);
            window = NULL;
            // Failed. Go on and retry non-3d window...
        } else {
            backend_winfo->sdl_win = window;
            backend_winfo->sdl_renderer = renderer;
            return 1;
        }
    }

    // Try to create a non-OpenGL, likely unaccelerated window:
    window = SDL_CreateWindow(
        title, SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED, width, height,
        SDL_WINDOW_RESIZABLE|
        (isfullscreen ? SDL_WINDOW_FULLSCREEN:0)|
        SDL_WINDOW_ALLOW_HIGHDPI);
    if (!window) {
        return 0;
    }
    renderer = SDL_CreateRenderer(window, -1,
        SDL_RENDERER_PRESENTVSYNC|SDL_RENDERER_SOFTWARE);
    if (!renderer) {
        SDL_DestroyWindow(window);
        window = NULL;
        return 0;
    }

    assert(window != NULL && renderer != NULL);
    backend_winfo->sdl_win = window;
    backend_winfo->sdl_renderer = renderer;
    return 1;
}

S3DHID void _s3d_sdl_GetWindowSDLRef(
        s3d_window *win,
        s3d_backend_windowing_wininfo *_backend_winfo,
        SDL_Window **out_win,
        SDL_Renderer **out_renderer) {
    s3d_backend_windowing_wininfo_sdl2 *backend_winfo =
        (s3d_backend_windowing_wininfo_sdl2 *)_backend_winfo;
    if (out_win != NULL)
        *out_win = backend_winfo->sdl_win;
    if (out_renderer != NULL)
        *out_renderer = backend_winfo->sdl_renderer;
}

S3DHID void _s3d_sdl_WarpMouse(
        s3d_backend_windowing *backend,
        s3d_window *win,
        s3d_backend_windowing_wininfo *_backend_winfo,
        s3dnum_t x, s3dnum_t y
        ) {
    s3d_backend_windowing_wininfo_sdl2 *backend_winfo =
        (s3d_backend_windowing_wininfo_sdl2 *)_backend_winfo;
    int wx = floor(x);
    int wy = floor(y);
    SDL_WarpMouseInWindow(backend_winfo->sdl_win, wx, wy);
}

S3DHID void _s3d_sdl_DestroyWinInfo(
        s3d_backend_windowing *backend, s3d_window *win,
        s3d_backend_windowing_wininfo *_backend_winfo
        ) {
    s3d_backend_windowing_wininfo_sdl2 *backend_winfo =
        (s3d_backend_windowing_wininfo_sdl2 *)_backend_winfo;
    if (!_backend_winfo)
        return;
    if (backend_winfo->sdl_renderer) {
        SDL_DestroyRenderer(backend_winfo->sdl_renderer);
    }
    if (backend_winfo->sdl_win) {
        SDL_DestroyWindow(backend_winfo->sdl_win);
    }
    free(backend_winfo);
}

s3d_backend_windowing *_singleton_sdl_backend = NULL;

S3DHID __attribute__((constructor)) static void _init_sdl2_backend() {
    s3d_backend_windowing *b = malloc(sizeof(*b));
    if (b) {
        memset(b, 0, sizeof(*b));
        b->kind = S3D_BACKEND_WINDOWING_SDL;
        b->CreateWinInfo = _s3d_sdl_CreateWinInfo;
        b->WarpMouse = _s3d_sdl_WarpMouse;
        b->DestroyWinInfo = _s3d_sdl_DestroyWinInfo;
        b->CreateWinObj = _s3d_sdl_CreateWinObj;
        b->WasWinObjCreated = _s3d_sdl_WasWinObjCreated;
        b->ResetMouseGrab = _s3d_sdl_ResetMouseGrab;
        b->WasWinObjCreated = _s3d_sdl_WasWinObjCreated;
        b->SetMouseGrabConstrained =
            _s3d_sdl_SetMouseGrabConstrained;
        b->SetMouseGrabInvisibleRelative =
            _s3d_sdl_SetMouseGrabInvisibleRelative;
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

