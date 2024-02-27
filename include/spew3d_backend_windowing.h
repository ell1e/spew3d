/* Copyright (c) 2024, ellie/@ell1e & Spew3D Team (see AUTHORS.md).

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

#ifndef SPEW3D_BACKEND_WINDOWING_H_
#define SPEW3D_BACKEND_WINDOWING_H_

typedef struct s3d_window s3d_window;
typedef struct s3d_backend_windowing s3d_backend_windowing;

typedef struct s3d_backend_windowing_wininfo
    s3d_backend_windowing_wininfo;

typedef struct s3d_backend_windowing {
    s3d_backend_windowing_wininfo *(*CreateWinInfo)(
        s3d_backend_windowing *backend, s3d_window *win
    );
    void (*DestroyWinInfo)(
        s3d_backend_windowing *backend, s3d_window *win,
        s3d_backend_windowing_wininfo *winfo
    );
    void (*WarpMouse)(s3d_backend_windowing *backend,
        s3d_window *win, s3d_backend_windowing_wininfo *wininfo,
        s3dnum_t x, s3dnum_t y);
    void *internal;
} s3d_backend_windowing;

#endif  // SPEW3D_BACKEND_WINDOWING_H_

