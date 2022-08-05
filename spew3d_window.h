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

#ifndef SPEW3D_WINDOW_H_
#define SPEW3D_WINDOW_H_

#include <stdint.h>
#include <SDL2/SDL.h>


typedef struct spew3d_point spew3d_point;

/// Convert coordinates from a mouse event supplied
/// by SDL2 into the 2d canvas draw units.
/// The resulting coordinates match what you'd supply to
/// something like SDL_RenderCopy, SDL_RenderFillRect, or
/// spew3d_texture_Draw to draw at the clicked spot.
/// This conversion is needed e.g. with a High-DPI window.
spew3d_point spew3d_window_EventPointToCanvasDrawPoint(
    int x, int y
);


/// Helper function for how wide the 2d canvas is (that may
/// or may not correspond to screen pixels) for the output
/// window. This unit is also used for SDL_RenderCopy,
/// SDL_RenderFillRect, or spew3d_texture_Draw.
/// This might differ from SDL_GetWindowSize e.g. for a
/// High-DPI window.
int32_t spew3d_window_CanvasDrawWidth();


/// Helper function for how tall the 2d canvas is (that may
/// or may not correspond to screen pixels) for the output
/// window. This unit is also used for SDL_RenderCopy,
/// SDL_RenderFillRect, or spew3d_texture_Draw.
/// This might differ from SDL_GetWindowSize e.g. for a
/// High-DPI window.
int32_t spew3d_window_CanvasDrawHeight();

#endif  // SPEW3D_WINDOW_H_
