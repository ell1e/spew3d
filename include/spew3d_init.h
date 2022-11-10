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

#ifndef SPEW3D_INIT_H_
#define SPEW3D_INIT_H_

#ifndef SPEW3D_OPTION_DISABLE_SDL
#include <SDL2/SDL.h>
#endif
#include <stdint.h>


enum {
    SPEW3D_INITFLAG_FULLSCREEN = 1,
    SPEW3D_INITFLAG_FORCE_OPENGL = 2,
    SPEW3D_INITFLAG_FORCE_SOFTWARE_RENDERED = 3
};

#ifndef SPEW3D_OPTION_DISABLE_SDL
/** This is a helper function if you don't want to do SDL_Init(),
 *  SDL_CreateWindow(), and SDL_CreateRenderer() manually. It will do
 *  those things for you in one go, then return a spew3d_ctx pointer
 *  which can be used for all Spew3D drawing and to return the
 *  underlying SDL2 window and renderer. */
spew3d_ctx *spew3d_init_CreateSDLWindowForMe(
    const char *window_title, int initflags
);
#endif  // #ifndef SPEW3D_OPTION_DISABLE_SDL

#endif  // SPEW3D_INIT_H_
