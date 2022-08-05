/* Spew3D is Copyright 2022 ell1e et al.

Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or
sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following
conditions:

The above copyright notice and this permission notice shall
be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY
KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS
OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifdef SPEW3D_IMPLEMENTATION

#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <SDL2/SDL.h>
#include <unistd.h>


SDL_Window *_internal_spew3d_outputwindow = NULL;
SDL_Renderer *_internal_spew3d_outputrenderer = NULL;


int spew3d_InitFromManualSDLInit(
        SDL_Window *window, SDL_Renderer *renderer
        ) {
    assert(_internal_spew3d_outputwindow == NULL);
    assert(_internal_spew3d_outputrenderer == NULL);
    if (_internal_spew3d_outputwindow != NULL ||
            _internal_spew3d_outputrenderer != NULL)
        return 0;
    assert(window != NULL && renderer != NULL);
    if (window == NULL || renderer == NULL)
        return 0;
    _internal_spew3d_outputwindow = window;
    _internal_spew3d_outputrenderer = renderer;
    return 1;
}

int spew3d_Init(
        const char *title, int initflags,
        SDL_Window **out_window, SDL_Renderer **out_renderer
        ) {
    SDL_Window *window = SDL_CreateWindow(
        title, SDL_WINDOWPOS_UNDEFINED,
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
                return 0;
        } else {
            *out_window = window;
            *out_renderer = renderer;
            return 1;
        }
    }
    window = SDL_CreateWindow(
        title, SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED, 800, 500,
        SDL_WINDOW_RESIZABLE|
        (((initflags & SPEW3D_INITFLAG_FULLSCREEN) != 0) ?
            SDL_WINDOW_FULLSCREEN:0)|
        SDL_WINDOW_ALLOW_HIGHDPI);
    if (!window)
        return 0;
    renderer = SDL_CreateRenderer(window, -1,
        SDL_RENDERER_PRESENTVSYNC|SDL_RENDERER_SOFTWARE);
    if (!renderer) {
        SDL_DestroyWindow(window);
        window = NULL;
        return 0;
    }

    if (!spew3d_InitFromManualSDLInit(window, renderer)) {
        SDL_DestroyWindow(window);
        window = NULL;
        SDL_DestroyRenderer(renderer);
        renderer = NULL;
        return 1;
    }

    *out_window = window;
    *out_renderer = renderer;
    return 1;
}

#endif  // SPEW3D_IMPLEMENTATION

