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

#ifndef SPEW3D_OPTION_DISABLE_SDL
#include <SDL2/SDL.h>
#endif
#include <stdint.h>

typedef struct spew3d_ctx {
    #ifndef SPEW3D_OPTION_DISABLE_SDL 
    SDL_Window *_sdl_outputwindow;
    SDL_Renderer *_sdl_outputrenderer;
    #endif
} spew3d_ctx;

spew3d_ctx *spew3d_ctx_New() {
    spew3d_ctx *ctx = malloc(sizeof(*ctx));
    if (!ctx)
        return NULL;
    memset(ctx, 0, sizeof(*ctx));
    return ctx;
}

#ifndef SPEW3D_OPTION_DISABLE_SDL
void spew3d_ctx_SetSDLWindowAndRenderer(
        spew3d_ctx *ctx, SDL_Window *w, SDL_Renderer *r
        ) {
    ctx->_sdl_outputwindow = w;
    ctx->_sdl_outputrenderer = r;
}
#endif

#ifndef SPEW3D_OPTION_DISABLE_SDL
void spew3d_ctx_GetSDLWindowAndRenderer(
        spew3d_ctx *ctx, SDL_Window **out_w,
        SDL_Renderer **out_r
        ) {
    if (out_w)
        *out_w = ctx->_sdl_outputwindow;
    if (out_r)
        *out_r = ctx->_sdl_outputrenderer;
}
#endif

#ifndef SPEW3D_OPTION_DISABLE_SDL
spew3d_ctx *spew3d_ctx_NewFromSDLRenderer(
        SDL_Window *w, SDL_Renderer *r
        ) {
    spew3d_ctx *ctx = spew3d_ctx_New();
    if (!ctx)
        return NULL;
    spew3d_ctx_SetSDLWindowAndRenderer(
        ctx, w, r
    );
    return ctx;
}
#endif

#endif  // SPEW3D_IMPLEMENTATION

