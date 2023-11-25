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

#ifdef SPEW3D_IMPLEMENTATION

#include <assert.h>
#include <inttypes.h>
#include <math.h>
#include <string.h>

#ifndef NDEBUG
static void __attribute__((constructor))
        _spew3d_math2dtest() {
    spew3d_point p = {0};
    p.x = 1 * S3D_NUMONE;
    p.y = 0;
    assert(S3D_ABS(spew3d_math2d_angle(&p) - S3D_DBLTONUM(0.0))
           < S3D_NUMONE / 10);
    p.x = 0;
    p.y = 1 * S3D_NUMONE;
    assert(S3D_ABS(spew3d_math2d_angle(&p) - S3D_DBLTONUM(90.0))
           < S3D_NUMONE / 10);
    p.x = 1 * S3D_NUMONE;
    p.y = -1 * S3D_NUMONE;
    assert(S3D_ABS(spew3d_math2d_angle(&p) - S3D_DBLTONUM(-45.0))
           < S3D_NUMONE / 10);

    p.x = 1 * S3D_NUMONE;
    p.y = 0;
    spew3d_math2d_rotate(&p, S3D_DBLTONUM(-90.0));
    assert(S3D_ABS(p.x - S3D_DBLTONUM(0.0)) < S3D_DBLTONUM(0.1));
    assert(S3D_ABS(p.y - S3D_DBLTONUM(-1.0)) < S3D_DBLTONUM(0.1));
}
#endif  // #ifndef NDEBUG


#endif  // SPEW3D_IMPLEMENTATION

