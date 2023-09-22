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

#ifndef SPEW3D_WINDOW_H_
#define SPEW3D_WINDOW_H_

#include <stdint.h>
#if !defined(SPEW3D_OPTION_DISABLE_SDL) &&\
        !defined(SPEW3D_OPTION_DISABLE_SDL_HEADER)
#include <SDL2/SDL.h>
#endif

typedef struct spew3d_window spew3d_window;
typedef struct spew3d_point spew3d_point;

#define SPEW3D_WINDOW_FLAG_FORCE_HIDDEN_VIRTUAL (0x1)
#define SPEW3D_WINDOW_FLAG_FULLSCREEN (0x2)
#define SPEW3D_WINDOW_FLAG_FORCE_NO_3D_ACCEL (0x4)
S3DEXP spew3d_window *spew3d_window_NewEx(
    const char *title, uint32_t flags,
    int32_t width, int32_t height
);

S3DEXP spew3d_window *spew3d_window_New(
    const char *title, uint32_t flags
);

#if !defined(SPEW3D_OPTION_DISABLE_SDL) &&\
        !defined(SPEW3D_OPTION_DISABLE_SDL_HEADER)
S3DEXP spew3d_window *spew3d_window_NewFromSDLWindowAndRenderer(
    uint32_t flags, SDL_Window *sdlwin, SDL_Renderer *sdlrenderer
);
#endif

#if !defined(SPEW3D_OPTION_DISABLE_SDL) &&\
        !defined(SPEW3D_OPTION_DISABLE_SDL_HEADER)
S3DEXP void spew3d_window_GetSDLWindowAndRenderer(
    spew3d_window *win, SDL_Window **out_sdlwindow,
    SDL_Renderer **out_sdlrenderer
);
#endif

typedef struct spew3d_point spew3d_point;
typedef struct spew3d_ctx spew3d_ctx;

/// Fill the inner area of the window with a solid color.
S3DEXP void spew3d_window_FillWithColor(
    spew3d_window *window,
    s3dnum_t red, s3dnum_t green, s3dnum_t blue
);

/// Show the latest drawn contents to the user.
S3DEXP void spew3d_window_PresentToScreen(spew3d_window *win);

/// Convert coordinates from high-level event coordinates
/// used for any mouse or touch events or any widgets, to
/// actual pixel coordinates on the window's 2d canvas.
/// The resulting coordinates match what you'd supply to
/// something like SDL_RenderCopy, SDL_RenderFillRect, or
/// spew3d_texture_DrawAtCanvasPixels.
/// This conversion is needed e.g. with a window DPI scale
/// other than 1.0.
S3DEXP void spew3d_window_PointToCanvasDrawPixels(
    spew3d_window *win, spew3d_point point,
    int32_t *x, int32_t *y
);

/// Helper function for how wide the 2d canvas is (that may
/// or may not correspond to screen pixels) for the output
/// window. This unit is also used for e.g. SDL_RenderCopy,
/// SDL_RenderFillRect, or spew3d_texture_DrawAtCanvasPixels.
/// This might differ from spew3d_window_GetWindowWidth/Height
/// if any window DPI scaling other than 1.0 which is common.
S3DEXP int32_t spew3d_window_GetCanvasDrawWidth(
    spew3d_window *win
);

/// Helper function for how tall the 2d canvas is (that may
/// or may not correspond to screen pixels) for the output
/// window. This unit is also used for e.g. SDL_RenderCopy,
/// SDL_RenderFillRect, or spew3d_texture_DrawAtCanvasPixels.
/// This might differ from spew3d_window_GetWindowSize
/// if any window DPI scaling other than 1.0 which is common.
S3DEXP int32_t spew3d_window_GetCanvasDrawHeight(
    spew3d_window *win
);

/// Get the titlebar string of a Spew3D window.
S3DEXP const char *spew3d_window_GetTitle(
    spew3d_window *win
);

/// Get the inner width of a window. Please note this is
/// in high-level event coordinates and therefore for any window
/// DPI scaling other than 1.0 will not be pixels. If you want
/// the pixel size then use spew3d_window_CanvasDrawWidth/Height.
S3DEXP spew3d_point spew3d_window_GetWindowSize(
    spew3d_window *win
);

#endif  // SPEW3D_WINDOW_H_
