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

#ifdef SPEW3D_IMPLEMENTATION

#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <SDL2/SDL.h>
#include <unistd.h>

spew3d_point spew3d_window_EventPointToCanvasDrawPoint(
        spew3d_ctx *ctx, int x, int y
        ) {
    #ifndef SPEW3D_OPTION_DISABLE_SDL
    SDL_Window *win = NULL;
    SDL_Renderer *renderer = NULL;
    spew3d_ctx_GetSDLWindowAndRenderer(ctx, &win, &renderer);
    if (win == NULL && renderer == NULL) {
        spew3d_point result;
        result.x = x;
        result.y = y;
        return result;
    }
    assert(win != NULL && renderer != NULL);
    #endif

    spew3d_point result;
    result.x = x;
    result.y = y;
    #ifdef SPEW3D_OPTION_DISABLE_SDL
    return result
    #else
    int w, h;
    SDL_RenderGetLogicalSize(renderer, &w, &h);
    if (w != 0 || h != 0)
        return result;  // handled by the renderer itself
    int ww, wh;
    SDL_GetWindowSize(win, &ww, &wh);
    if (!SDL_GetRendererOutputSize(
            renderer, &w, &h
            ))
        return result;
    double newx = ((double)x) * ((double)w/(double)ww);
    double newy = ((double)y) * ((double)h/(double)wh);
    result.x = round(newx);
    result.y = round(newy);
    return result;
    #endif  // #ifndef SPEW3D_OPTION_DISABLE_SDL
}

int32_t spew3d_window_CanvasDrawWidth(spew3d_ctx *ctx) {
    #ifdef SPEW3D_OPTION_DISABLE_SDL
    return 1;
    #else
    SDL_Renderer *renderer = NULL;
    spew3d_ctx_GetSDLWindowAndRenderer(ctx, NULL, &renderer);
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
    #endif  // #ifndef SPEW3D_OPTION_DISABLE_SDL
}

int32_t spew3d_window_CanvasDrawHeight(spew3d_ctx *ctx) {
    #ifdef SPEW3D_OPTION_DISABLE_SDL
    return 1;
    #else
    SDL_Renderer *renderer = NULL;
    spew3d_ctx_GetSDLWindowAndRenderer(ctx, NULL, &renderer);
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
    #endif  // #ifndef SPEW3D_OPTION_DISABLE_SDL
}

#endif  // SPEW3D_IMPLEMENTATION

