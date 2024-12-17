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

#if defined(SPEW3D_IMPLEMENTATION) && \
    SPEW3D_IMPLEMENTATION != 0
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

S3DHID int _s3d_sdl_IsWindowConsideredFocused(
        s3d_backend_windowing *backend, s3d_window *win,
        s3d_backend_windowing_wininfo *_backend_winfo
        ) {
    s3d_backend_windowing_wininfo_sdl2 *backend_winfo = (
        (s3d_backend_windowing_wininfo_sdl2 *)_backend_winfo
    );
    SDL_Renderer *renderer = backend_winfo->sdl_renderer;
    SDL_Window *sdlwin = backend_winfo->sdl_win;

    if (!sdlwin) {
        return 0;
    }
    uint32_t flags = SDL_GetWindowFlags(sdlwin);
    return ((flags & SDL_WINDOW_INPUT_FOCUS) != 0);
}

S3DHID int _s3d_sdl_GetWindowGeometry(
        s3d_backend_windowing *backend, s3d_window *win,
        s3d_backend_windowing_wininfo *_backend_winfo,
        uint32_t *out_canvaswidth, uint32_t *out_canvasheight,
        uint32_t *out_windowwidth, uint32_t *out_windowheight
        ) {
    s3d_backend_windowing_wininfo_sdl2 *backend_winfo = (
        (s3d_backend_windowing_wininfo_sdl2 *)_backend_winfo
    );
    SDL_Renderer *renderer = backend_winfo->sdl_renderer;
    SDL_Window *sdlwin = backend_winfo->sdl_win;

    if (!renderer)
        return 0;

    int w, h;
    SDL_RenderGetLogicalSize(renderer, &w, &h);
    if (w == 0 && h == 0) {
        if (SDL_GetRendererOutputSize(
                renderer, &w, &h
                ) != 0) {
            w = 1;
            h = 1;
        }
    }
    if (w < 1)
        w = 1;
    if (h < 1)
        h = 1;
    if (out_canvaswidth != NULL)
        *out_canvaswidth = w;
    if (out_canvasheight != NULL)
        *out_canvasheight = h;
    int ww, wh;
    SDL_GetWindowSize(sdlwin, &ww, &wh);
    if (ww < 1)
        ww = 1;
    if (wh < 1)
        wh = 1;
    if (out_windowwidth != NULL)
        *out_windowwidth = ww;
    if (out_windowheight != NULL)
        *out_windowheight = wh;
    return 1;
}

S3DHID void _s3d_sdl_FillWindowWithColor(
        s3d_backend_windowing *backend, s3d_window *win,
        s3d_backend_windowing_wininfo *_backend_winfo,
        s3dnum_t red, s3dnum_t green, s3dnum_t blue
        ) {
    s3d_backend_windowing_wininfo_sdl2 *backend_winfo = (
        (s3d_backend_windowing_wininfo_sdl2 *)_backend_winfo
    );
    SDL_Renderer *renderer = backend_winfo->sdl_renderer;
    SDL_Window *sdlwin = backend_winfo->sdl_win;

    if (!sdlwin) {
        return;
    }
    double redv = fmax(0.0, fmin(255.0,
        (double)red * 256.0));
    double greenv = fmax(0.0, fmin(255.0,
        (double)green * 256.0));
    double bluev = fmax(0.0, fmin(255.0,
        (double)blue * 256.0));
    SDL_SetRenderDrawColor(
        renderer, redv, greenv, bluev, 255
    );
    SDL_RenderClear(renderer);
}

S3DHID void _s3d_sdl_PresentWindowToScreen(
        s3d_backend_windowing *backend, s3d_window *win,
        s3d_backend_windowing_wininfo *_backend_winfo
        ) {
    s3d_backend_windowing_wininfo_sdl2 *backend_winfo = (
        (s3d_backend_windowing_wininfo_sdl2 *)_backend_winfo
    );
    SDL_Renderer *renderer = backend_winfo->sdl_renderer;
    SDL_Window *sdlwin = backend_winfo->sdl_win;

    if (!sdlwin) {
        return;
    }
    SDL_RenderPresent(renderer);
}

S3DHID int _s3d_sdl_DrawSpriteAtPixels(
        s3d_backend_windowing *backend, s3d_window *win,
        s3d_backend_windowing_wininfo *_backend_winfo,
        s3d_backend_windowing_gputex *gputex,
        int32_t x, int32_t y, s3dnum_t scale, s3dnum_t angle,
        s3dnum_t tint_red, s3dnum_t tint_green, s3dnum_t tint_blue,
        s3dnum_t transparency, int centered,
        int withalphachannel
        ) {
    s3d_backend_windowing_wininfo_sdl2 *backend_winfo = (
        (s3d_backend_windowing_wininfo_sdl2 *)_backend_winfo
    );
    SDL_Renderer *renderer = backend_winfo->sdl_renderer;
    SDL_Window *sdlwin = backend_winfo->sdl_win;
    SDL_Texture *tex = (SDL_Texture *)gputex;

    assert(backend_winfo != NULL);
    assert(renderer != NULL);

    if (!tex) {
        return 1;
    }
    uint32_t texwidth, texheight;
    if (SDL_QueryTexture(tex, NULL, NULL, &texwidth, &texheight) != 0) {
        assert(0);
        return 0;
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
        ((int32_t)((double)texwidth * scale) / 2) : 0);
    r.y = y - (centered ?
        ((int32_t)((double)texheight * scale) / 2) : 0);
    r.w = round((double)texwidth * scale);
    r.h = round((double)texheight * scale);
    SDL_SetRenderDrawBlendMode(renderer,
        SDL_BLENDMODE_BLEND);
    if (SDL_RenderCopyEx(renderer, tex, NULL, &r,
            -angle, NULL, SDL_FLIP_NONE) != 0)
        return 1;
    if (SDL_SetRenderDrawColor(renderer,
            old_r, old_g, old_b, old_a) != 0)
        return 1;
    return 1;
}

S3DHID void _s3d_sdl_DestroyGPUTexture(
        s3d_backend_windowing *backend, s3d_window *win,
        s3d_backend_windowing_wininfo *backend_winfo,
        s3d_backend_windowing_gputex *tex
        ) {
    SDL_Texture *sdltex = (SDL_Texture *)tex;
    SDL_DestroyTexture(sdltex);
}

S3DHID s3d_backend_windowing_gputex *_s3d_sdl_CreateGPUTexture(
        s3d_backend_windowing *backend, s3d_window *win,
        s3d_backend_windowing_wininfo *_backend_winfo,
        void *pixel_rgba_data, uint32_t w, uint32_t h,
        int set_alpha_solid
        ) {
    s3d_backend_windowing_wininfo_sdl2 *backend_winfo =
        (s3d_backend_windowing_wininfo_sdl2 *)_backend_winfo;
    SDL_Renderer *renderer = backend_winfo->sdl_renderer;
    SDL_Window *sdlwin = backend_winfo->sdl_win;

    SDL_Surface *s = SDL_CreateRGBSurfaceFrom(
        pixel_rgba_data, w, h,
        32, w * 4, 0x000000ff,
        0x0000ff00, 0x00ff0000,
        0xff000000
    );
    if (!s)
        return 0;
    if (set_alpha_solid) {
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
    SDL_Texture *tex = SDL_CreateTextureFromSurface(
        renderer, s
    );
    SDL_FreeSurface(s);
    #if defined(DEBUG_SPEW3D_TEXTURE)
    fprintf(stderr,
        "spew3d_backend_windowing_sdl.c: debug: "
        "_s3d_sdl_CreateGPUTexture(): "
        "Created GPU texture SDL_Texture*==%p\n",
        tex);
    #endif
    return (s3d_backend_windowing_gputex *)tex;
}

S3DHID int _s3d_sdl_DrawPolygonAtPixels(
        s3d_backend_windowing *backend, s3d_window *win,
        s3d_backend_windowing_wininfo *_backend_winfo,
        s3d_backend_windowing_gputex *tex,
        s3d_pos *vertices, s3d_point *tex_points,
        s3d_color *colors
        ) {
    s3d_backend_windowing_wininfo_sdl2 *backend_winfo =
        (s3d_backend_windowing_wininfo_sdl2 *)_backend_winfo;
    SDL_Renderer *renderer = backend_winfo->sdl_renderer;
    SDL_Window *sdlwin = backend_winfo->sdl_win;
    assert(backend_winfo != NULL);
    assert(renderer != NULL);

    SDL_Vertex vertex_1 = {
        {vertices[0].y, vertices[0].z},
        {255.0 * colors[0].red, 255.0 * colors[0].red,
         255.0 * colors[0].blue, 255.0 * colors[0].alpha},
        {tex_points[0].x, tex_points[0].y}
    };
    SDL_Vertex vertex_2 = {
        {vertices[1].y, vertices[1].z},
        {255.0 * colors[1].red, 255.0 * colors[1].red,
         255.0 * colors[1].blue, 255.0 * colors[1].alpha},
        {tex_points[1].x, tex_points[1].y}
    };
    SDL_Vertex vertex_3 = {
        {vertices[2].y, vertices[2].z},
        {255.0 * colors[2].red, 255.0 * colors[2].red,
         255.0 * colors[2].blue, 255.0 * colors[2].alpha},
        {tex_points[2].x, tex_points[2].y}
    };
    SDL_Vertex sdlvertices[] = {
        vertex_1,
        vertex_2,
        vertex_3
    };
    if (SDL_SetRenderDrawColor(renderer,
            255, 255, 255, 255) != 0 ||
            SDL_SetRenderDrawBlendMode(renderer,
            SDL_BLENDMODE_BLEND) != 0) {
        return 0;
    }
    SDL_RenderGeometry(renderer,
        (SDL_Texture *)tex, sdlvertices, 3, NULL, 0);
    return 1;
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
        b->IsWindowConsideredFocused =
            _s3d_sdl_IsWindowConsideredFocused;
        b->PresentWindowToScreen =
            _s3d_sdl_PresentWindowToScreen;
        b->GetWindowGeometry =
            _s3d_sdl_GetWindowGeometry;
        b->FillWindowWithColor =
            _s3d_sdl_FillWindowWithColor;

        b->supports_gpu_textures = 1;
        b->CreateGPUTexture = _s3d_sdl_CreateGPUTexture;
        b->DestroyGPUTexture = _s3d_sdl_DestroyGPUTexture;
        b->DrawSpriteAtPixels = _s3d_sdl_DrawSpriteAtPixels;
        b->DrawPolygonAtPixels = _s3d_sdl_DrawPolygonAtPixels;

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

