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

#ifndef SPEW3D_INIT_H_
#define SPEW3D_INIT_H_

#include <stdint.h>
#include <SDL2/SDL.h>


enum {
    SPEW3D_INITFLAG_FULLSCREEN = 1,
    SPEW3D_INITFLAG_FORCE_OPENGL = 2,
    SPEW3D_INITFLAG_FORCE_SOFTWARE_RENDERED = 3
};

int spew3d_Init(
    const char *title, int initflags,
    SDL_Window **out_window, SDL_Renderer **out_renderer
);

int spew3d_InitFromManualSDLInit(
    SDL_Window *window, SDL_Renderer *renderer
);

#endif  // SPEW3D_INIT_H_
