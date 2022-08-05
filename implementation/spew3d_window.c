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


extern SDL_Window *_internal_spew3d_outputwindow;
extern SDL_Renderer *_internal_spew3d_outputrenderer;


spew3d_point spew3d_window_EventPointToCanvasDrawPoint(
        int x, int y
        ) {
    spew3d_point result;
    result.x = x;
    result.y = y;
    if (!_internal_spew3d_outputrenderer)
        return result;
    int w, h;
    SDL_RenderGetLogicalSize(
        _internal_spew3d_outputrenderer, &w, &h
    );
    if (w != 0 || h != 0)
        return result;  // handled by the renderer itself
    int ww, wh;
    SDL_GetWindowSize(_internal_spew3d_outputwindow,
        &ww, &wh
    );
    if (!SDL_GetRendererOutputSize(
            _internal_spew3d_outputrenderer,
            &w, &h))
        return result;
    double newx = ((double)x) * ((double)w/(double)ww);
    double newy = ((double)y) * ((double)h/(double)wh);
    result.x = round(newx);
    result.y = round(newy);
    return result;
}


int32_t spew3d_window_CanvasDrawWidth() {
    if (!_internal_spew3d_outputrenderer)
        return 1;
    int w, h;
    SDL_RenderGetLogicalSize(
        _internal_spew3d_outputrenderer, &w, &h
    );
    if (w == 0 && h == 0)
        if (!SDL_GetRendererOutputSize(
                _internal_spew3d_outputrenderer,
                &w, &h))
            return 1;
    if (w < 1)
        return w;
    return w;
}


int32_t spew3d_window_CanvasDrawHeight() {
    if (!_internal_spew3d_outputrenderer)
        return 1;
    int w, h;
    SDL_RenderGetLogicalSize(
        _internal_spew3d_outputrenderer, &w, &h
    );
    if (w == 0 && h == 0)
        if (!SDL_GetRendererOutputSize(
                _internal_spew3d_outputrenderer,
                &w, &h))
            return 1;
    if (h < 1)
        return h;
    return h;
}

#endif  // SPEW3D_IMPLEMENTATION

