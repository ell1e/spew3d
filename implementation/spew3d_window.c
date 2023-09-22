/* Copyright (c) 2020-2023, ellie/@ell1e & Spew3D Team (see AUTHORS.md).

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

typedef struct spew3d_window {
    uint32_t flags;
    #ifndef SPEW3D_OPTION_DISABLE_SDL
    int owns_sdl_window;
    SDL_Window *_sdl_outputwindow;
    int owns_sdl_renderer;
    SDL_Renderer *_sdl_outputrenderer;
    #endif
    struct virtualwin {
        spew3d_texture_t canvas;
        char *title;
        s3dnum_t dpiscale;
        int32_t canvaswidth, canvasheight;
    } virtualwin;
    int32_t width, height;
} spew3d_window;

static spew3d_window *spew3d_window_NewExEx(
        const char *title, uint32_t flags,
        int dontinitactualwindow, int32_t width, int32_t height
        ) {
    if (!_internal_spew3d_InitSDLGraphics())
        return NULL;

    spew3d_window *win = malloc(sizeof(*win));
    if (!win)
        return NULL;
    memset(win, 0, sizeof(*win));

    if (width <= 0) width = 800;
    if (height <= 0) height = 500;
    win->width = width;
    win->height = height;
    win->flags = flags;

    if (!dontinitactualwindow) {
        if ((flags & SPEW3D_WINDOW_FLAG_FORCE_HIDDEN_VIRTUAL) == 0) {
            #ifndef SPEW3D_OPTION_DISABLE_SDL
            int isfullscreen = (
                (flags & SPEW3D_WINDOW_FLAG_FULLSCREEN) != 0
            );

            // Try to create a 3d accelerated OpenGL window:
            SDL_Window *window = SDL_CreateWindow(
                title, SDL_WINDOWPOS_UNDEFINED,
                SDL_WINDOWPOS_UNDEFINED,
                (isfullscreen ? 800 : width),
                (isfullscreen ? 500 : height),
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
                    win->owns_sdl_window = 1;
                    win->_sdl_outputwindow = window;
                    win->owns_sdl_renderer = 1;
                    win->_sdl_outputrenderer = renderer;
                    win->width = 0;
                    win->height = 0;
                    return win;
                }
            }

            // Try to create a non-OpenGL, likely unaccelerated window:
            window = SDL_CreateWindow(
                title, SDL_WINDOWPOS_UNDEFINED,
                SDL_WINDOWPOS_UNDEFINED,
                (isfullscreen ? 800 : width),
                (isfullscreen ? 500 : height),
                SDL_WINDOW_RESIZABLE|
                (isfullscreen ? SDL_WINDOW_FULLSCREEN:0)|
                SDL_WINDOW_ALLOW_HIGHDPI);
            if (!window) {
                free(win);
                return NULL;
            }
            renderer = SDL_CreateRenderer(window, -1,
                SDL_RENDERER_PRESENTVSYNC|SDL_RENDERER_SOFTWARE);
            if (!renderer) {
                SDL_DestroyWindow(window);
                window = NULL;
                free(win);
                return NULL;
            }

            assert(window != NULL && renderer != NULL);
            win->owns_sdl_window = 1;
            win->_sdl_outputwindow = window;
            win->owns_sdl_renderer = 1;
            win->_sdl_outputrenderer = renderer;
            win->width = 0;
            win->height = 0;
            return win;
            #endif  // ifndef SPEW3D_OPTION_DISABLE_SDL
        }

        // Create a virtual window:
        win->virtualwin.canvas = spew3d_texture_NewWritable(
            NULL, width, height
        );
        if (win->virtualwin.canvas == 0) {
            free(win);
            return win;
        }
        win->virtualwin.title = strdup(title);
        if (!win->virtualwin.title) {
            spew3d_texture_Destroy(win->virtualwin.canvas);
            free(win);
            return NULL;
        }
        win->virtualwin.dpiscale = 1.0;
        win->width = width;
        win->height = height;
    }

    return win;
}

S3DEXP void spew3d_window_PresentToScreen(spew3d_window *win) {
    #ifndef SPEW3D_OPTION_DISABLE_SDL
    if (win->virtualwin.canvas == 0) {
        SDL_RenderPresent(win->_sdl_outputrenderer);
        return;
    }
    #endif
}

S3DEXP void spew3d_window_FillWithColor(
        spew3d_window *win,
        s3dnum_t red, s3dnum_t green, s3dnum_t blue
        ) {
    #ifndef SPEW3D_OPTION_DISABLE_SDL
    if (win->virtualwin.canvas == 0) {
        double redv = fmax(0.0, fmin(255.0,
            S3D_NUMTODBL(red) * 256.0));
        double greenv = fmax(0.0, fmin(255.0,
            S3D_NUMTODBL(green) * 256.0));
        double bluev = fmax(0.0, fmin(255.0,
            S3D_NUMTODBL(blue) * 256.0));
        SDL_SetRenderDrawColor(
            win->_sdl_outputrenderer, redv, greenv, bluev, 255
        );
        SDL_RenderClear(win->_sdl_outputrenderer);
        return;
    }
    #endif
    assert(win->virtualwin.canvas != 0);
    // FIXME: implement this later
}

S3DEXP spew3d_window *spew3d_window_New(
        const char *title, uint32_t flags
        ) {
    return spew3d_window_NewExEx(title, flags, 0, 0, 0);
}

S3DEXP spew3d_window *spew3d_window_NewEx(
        const char *title, uint32_t flags,
        int32_t width, int32_t height
        ) {
    return spew3d_window_NewExEx(
        title, flags, 0, width, height);
}

#if !defined(SPEW3D_OPTION_DISABLE_SDL)
S3DEXP spew3d_window *spew3d_window_NewFromSDLWindowAndRenderer(
        uint32_t flags, SDL_Window *w, SDL_Renderer *r
        ) {
    if (!w || !r)
        return NULL;
    if ((flags & SPEW3D_WINDOW_FLAG_FORCE_HIDDEN_VIRTUAL) != 0)
        return NULL;  // This isn't allowed.
    spew3d_window *win = spew3d_window_NewExEx(NULL, flags, 1, 0, 0);
    win->_sdl_outputwindow = w;
    win->_sdl_outputrenderer = r;
    return win;
}
#endif

#if !defined(SPEW3D_OPTION_DISABLE_SDL) &&\
        defined(SPEW3D_OPTION_DISABLE_SDL_HEADER)
// This won't be in the header, so define it here:
S3DEXP void spew3d_window_GetSDLWindowAndRenderer(
    spew3d_window *win, SDL_Window **out_w,
    SDL_Renderer **out_r
);
#endif

#ifndef SPEW3D_OPTION_DISABLE_SDL
S3DEXP void spew3d_window_GetSDLWindowAndRenderer(
        spew3d_window *win, SDL_Window **out_w,
        SDL_Renderer **out_r
        ) {
    if (out_w) *out_w = win->_sdl_outputwindow;
    if (out_r) *out_r = win->_sdl_outputrenderer;
}
#endif

S3DEXP spew3d_point spew3d_window_GetWindowSize(
        spew3d_window *win
        ) {
    int w = 0;
    int h = 0;
    #ifdef SPEW3D_OPTION_DISABLE_SDL
    assert(win->virtualwin.canvas != 0);
    #endif
    if (win->virtualwin.canvas != 0) {
        w = win->width;
        h = win->height;
    } else {
        #ifndef SPEW3D_OPTION_DISABLE_SDL
        SDL_Window *sdlwin = NULL;
        SDL_Renderer *renderer = NULL;
        spew3d_window_GetSDLWindowAndRenderer(win, &sdlwin, &renderer);
        assert(sdlwin != NULL && renderer != NULL);
        SDL_GetWindowSize(sdlwin, &w, &h);
        #endif
    }
    spew3d_point result;
    result.x = ((s3dnum_t)w) * S3D_NUMONE;
    result.y = ((s3dnum_t)h) * S3D_NUMONE;
    return result;
}

S3DEXP const char *spew3d_window_GetTitle(
        spew3d_window *win
        ) {
    #ifndef SPEW3D_OPTION_DISABLE_SDL
    SDL_Window *sdlwin = NULL;
    SDL_Renderer *renderer = NULL;
    spew3d_window_GetSDLWindowAndRenderer(win, &sdlwin, &renderer);
    if (sdlwin == NULL || renderer == NULL) {
        return win->virtualwin.title;
    }
    assert(win->virtualwin.canvas == 0);
    return SDL_GetWindowTitle(sdlwin);
    #else
    return win->virtualwin.title;
    #endif
}

S3DEXP void spew3d_window_PointToCanvasDrawPixels(
        spew3d_window *win, spew3d_point point,
        int32_t *x, int32_t *y
        ) {
    #ifndef SPEW3D_OPTION_DISABLE_SDL
    SDL_Window *sdlwin = NULL;
    SDL_Renderer *renderer = NULL;
    spew3d_window_GetSDLWindowAndRenderer(win, &sdlwin, &renderer);
    if (sdlwin == NULL || renderer == NULL) {
        s3dint128_t x128 = S3D_INT128MULT(point.x, win->virtualwin.dpiscale);
        s3dint128_t y128 = S3D_INT128MULT(point.y, win->virtualwin.dpiscale);
        *x = S3D_INT128DIV(x128, S3D_INT128FROM64(S3D_NUMONE)); 
        *y = S3D_INT128DIV(y128, S3D_INT128FROM64(S3D_NUMONE));
        return;
    }
    assert(win->virtualwin.canvas == 0);
    #endif

    #ifdef SPEW3D_OPTION_DISABLE_SDL
    *x = 0;
    *y = 0;
    return;
    #else
    int w, h;
    SDL_RenderGetLogicalSize(renderer, &w, &h);
    if (w != 0 || h != 0) {
        *x = S3D_NUMTODBL(point.x);
        *y = S3D_NUMTODBL(point.y);
        return;
    }
    int ww, wh;
    SDL_GetWindowSize(sdlwin, &ww, &wh);
    if (!SDL_GetRendererOutputSize(
            renderer, &w, &h
            )) {
        *x = S3D_NUMTODBL(point.x);
        *y = S3D_NUMTODBL(point.y);
        return;
    }
    *x = round(((double)S3D_NUMTODBL(point.x)) *
        ((double)w/(double)ww));
    *y = round(((double)S3D_NUMTODBL(point.y)) *
        ((double)h/(double)wh));
    #endif  // #ifndef SPEW3D_OPTION_DISABLE_SDL
}

S3DEXP int32_t spew3d_window_GetCanvasDrawWidth(spew3d_window *win) {
    if (
            #ifndef SPEW3D_OPTION_DISABLE_SDL
            win->virtualwin.canvas != 0
            #else
            1
            #endif
            ) {
        return win->virtualwin.canvaswidth;
    }
    #ifndef SPEW3D_OPTION_DISABLE_SDL
    SDL_Renderer *renderer = NULL;
    spew3d_window_GetSDLWindowAndRenderer(win, NULL, &renderer);
    if (!renderer)
        return 1;
    int w, h;
    SDL_RenderGetLogicalSize(renderer, &w, &h);
    if (w == 0 && h == 0)
        if (SDL_GetRendererOutputSize(
                renderer, &w, &h
                ) != 0)
            return 1;
    if (w < 1)
        return w;
    return w;
    #else
    return 0;
    #endif  // #ifndef SPEW3D_OPTION_DISABLE_SDL
}

S3DEXP int32_t spew3d_window_GetCanvasDrawHeight(spew3d_window *win) {
    if (
            #ifndef SPEW3D_OPTION_DISABLE_SDL
            win->virtualwin.canvas != 0
            #else
            1
            #endif
            ) {
        return win->virtualwin.canvasheight;
    }
    #ifndef SPEW3D_OPTION_DISABLE_SDL
    SDL_Renderer *renderer = NULL;
    spew3d_window_GetSDLWindowAndRenderer(win, NULL, &renderer);
    if (!renderer)
        return 1;
    int w, h;
    SDL_RenderGetLogicalSize(renderer, &w, &h);
    if (w == 0 && h == 0)
        if (SDL_GetRendererOutputSize(
                renderer, &w, &h
                ) != 0)
            return 1;
    if (h < 1)
        return h;
    return h;
    #else
    return 0;
    #endif  // #ifndef SPEW3D_OPTION_DISABLE_SDL
}

#endif  // SPEW3D_IMPLEMENTATION

