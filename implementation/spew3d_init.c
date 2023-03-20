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

S3DHID int _internal_spew3d_InitGraphics() {
    #ifndef SPEW3D_OPTION_DISABLE_SDL
    SDL_SetHintWithPriority(
        SDL_HINT_FRAMEBUFFER_ACCELERATION, "0",
        SDL_HINT_OVERRIDE);
    SDL_SetHintWithPriority(
        SDL_HINT_WINDOWS_DPI_SCALING, "1",
        SDL_HINT_OVERRIDE);
    if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER|
            SDL_INIT_EVENTS) != 0) {
        return 0;
    }
    return 1;
    #else
    return 1;
    #endif
}

#ifndef SPEW3D_OPTION_DISABLE_SDL
S3DEXP spew3d_ctx *spew3d_CreateSDLWindowForMe(
        const char *window_title, int initflags
        ) {
    if (!_internal_spew3d_InitGraphics())
        return 0;

    SDL_Window *window = SDL_CreateWindow(
        window_title, SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED, 800, 500,
        SDL_WINDOW_RESIZABLE|
        (((initflags & SPEW3D_INITFLAG_FULLSCREEN) != 0) ?
            SDL_WINDOW_FULLSCREEN:0)|
        (((initflags & SPEW3D_INITFLAG_FORCE_SOFTWARE_RENDERED) == 0) ?
            SDL_WINDOW_OPENGL:0)|
        SDL_WINDOW_ALLOW_HIGHDPI);
    if (!window && (((initflags & SPEW3D_INITFLAG_FORCE_OPENGL) != 0)))
        return 0;
    SDL_Renderer *renderer = NULL;
    if (window) {
        renderer = SDL_CreateRenderer(window, -1,
            SDL_RENDERER_PRESENTVSYNC|
            (((initflags & SPEW3D_INITFLAG_FORCE_SOFTWARE_RENDERED) == 0) ?
                SDL_RENDERER_ACCELERATED:SDL_RENDERER_SOFTWARE));
        if (!renderer) {
            SDL_DestroyWindow(window);
            window = NULL;
            if ((initflags & SPEW3D_INITFLAG_FORCE_OPENGL) != 0)
                return NULL;
        } else {
            spew3d_ctx *ctx = spew3d_ctx_NewFromSDLRenderer(
                window, renderer
            );
            if (!ctx) {
                SDL_DestroyWindow(window);
                window = NULL;
                SDL_DestroyRenderer(renderer);
                renderer = NULL;
                return NULL;
            }
            return ctx;
        }
    }
    window = SDL_CreateWindow(
        window_title, SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED, 800, 500,
        SDL_WINDOW_RESIZABLE|
        (((initflags & SPEW3D_INITFLAG_FULLSCREEN) != 0) ?
            SDL_WINDOW_FULLSCREEN:0)|
        SDL_WINDOW_ALLOW_HIGHDPI);
    if (!window)
        return NULL;
    renderer = SDL_CreateRenderer(window, -1,
        SDL_RENDERER_PRESENTVSYNC|SDL_RENDERER_SOFTWARE);
    if (!renderer) {
        SDL_DestroyWindow(window);
        window = NULL;
        return NULL;
    }

    assert(window != NULL && renderer != NULL);
    spew3d_ctx *ctx = spew3d_ctx_NewFromSDLRenderer(
        window, renderer
    );
    if (!ctx) {
        SDL_DestroyWindow(window);
        window = NULL;
        SDL_DestroyRenderer(renderer);
        renderer = NULL;
        return NULL;
    }

    return ctx;
}
#endif  // #ifndef SPEW3D_OPTION_DISABLE_SDL

#endif  // SPEW3D_IMPLEMENTATION

