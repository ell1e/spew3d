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

#if defined(SPEW3D_IMPLEMENTATION) && \
    SPEW3D_IMPLEMENTATION != 0

#include <assert.h>
#include <stdint.h>
#include <string.h>
#ifndef SPEW3D_OPTION_DISABLE_SDL
#include <SDL2/SDL.h>
#endif
#include <unistd.h>

int _global_graphics_init_done = 0;
int _global_audio_init_done = 0;

S3DHID int _internal_spew3d_InitSDLGraphics() {
    if (_global_graphics_init_done)
        return 1;
    #ifndef SPEW3D_OPTION_DISABLE_SDL
    SDL_SetHintWithPriority(
        SDL_HINT_FRAMEBUFFER_ACCELERATION, "0",
        SDL_HINT_DEFAULT);
    SDL_SetHintWithPriority(
        SDL_HINT_RENDER_SCALE_QUALITY, "0",
        SDL_HINT_OVERRIDE);
    SDL_SetHintWithPriority(
        SDL_HINT_MOUSE_AUTO_CAPTURE, "0",
        SDL_HINT_OVERRIDE
    );
    SDL_SetHintWithPriority(
        SDL_HINT_GRAB_KEYBOARD, "0",
        SDL_HINT_OVERRIDE
    );
    SDL_SetHintWithPriority(
        SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "0",
        SDL_HINT_OVERRIDE
    );
    SDL_SetHintWithPriority(
        SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1",
        SDL_HINT_OVERRIDE
    );
    #ifdef SDL_HINT_WINDOWS_DPI_SCALING
    SDL_SetHintWithPriority(
        SDL_HINT_WINDOWS_DPI_SCALING, "1",
        SDL_HINT_OVERRIDE);
    #endif
    if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER|
            SDL_INIT_EVENTS) != 0) {
        return 0;
    }
    #endif
    _global_graphics_init_done = 1;
    return 1;
}

S3DHID int _internal_spew3d_InitAudio() {
    if (_global_audio_init_done)
        return 1;
    #ifndef SPEW3D_OPTION_DISABLE_SDL
    if (SDL_Init(SDL_INIT_AUDIO|SDL_INIT_TIMER|
            SDL_INIT_EVENTS) != 0) {
        return 0;
    }
    #endif
    _global_audio_init_done = 1;
    return 1;
}

#endif  // SPEW3D_IMPLEMENTATION

