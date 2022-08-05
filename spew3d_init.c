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


int spew3d_Init(SDL_Window *window, SDL_Renderer *renderer) {
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

#endif  // SPEW3D_IMPLEMENTATION

