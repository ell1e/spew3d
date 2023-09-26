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
#include <math.h>
#include <SDL2/SDL.h>
#include <stdio.h>
#include <string.h>


#ifndef NDEBUG
static void __attribute__((constructor))
        _spew3d_math3dtest() {
    spew3d_rotation r;
    spew3d_pos p;
    p.x = 1;
    p.y = 0;
    p.z = 0;
    r.hori = 90;
    r.verti = 0;
    r.roll = 0;
    spew3d_math3d_rotate(&p, &r);
    assert(fabs(p.x - (0)) <= 0.01);
    assert(fabs(p.y - (1)) <= 0.01);
    assert(fabs(p.z - (0)) <= 0.01);

    p.x = 1;
    p.y = 0;
    p.z = 1;
    r.hori = 90;
    r.verti = 0;
    r.roll = 0;
    spew3d_math3d_rotate(&p, &r);
    assert(fabs(p.x - (0)) <= 0.01);
    assert(fabs(p.y - (1)) <= 0.01);
    assert(fabs(p.z - (1)) <= 0.01);

    p.x = 1;
    p.y = 1;
    p.z = 1;
    r.hori = 90;
    r.verti = 0;
    r.roll = 0;
    spew3d_math3d_rotate(&p, &r);
    assert(fabs(p.x - (-1)) <= 0.01);
    assert(fabs(p.y - (1)) <= 0.01);
    assert(fabs(p.z - (1)) <= 0.01);

    p.x = 1;
    p.y = 0;
    p.z = 1;
    r.hori = 90;
    r.verti = 0;
    r.roll = 0;
    spew3d_math3d_rotate(&p, &r);
    assert(fabs(p.x - (0)) <= 0.01);
    assert(fabs(p.y - (1)) <= 0.01);
    assert(fabs(p.z - (1)) <= 0.01);

    p.x = 1;
    p.y = 0;
    p.z = 0;
    r.hori = 45;
    r.verti = 45;
    r.roll = 0;
    spew3d_math3d_rotate(&p, &r);
    assert(fabs(p.x - (0.5)) <= 0.01);
    assert(fabs(p.y - (0.5)) <= 0.01);
    assert(fabs(p.z - (0.707107)) <= 0.01);

    p.x = 1;
    p.y = 1;
    p.z = 0;
    r.hori = 0;
    r.verti = 45;
    r.roll = 0;
    spew3d_math3d_rotate(&p, &r);
    assert(fabs(p.x - (0.707107)) <= 0.01);
    assert(fabs(p.y - (1)) <= 0.01);
    assert(fabs(p.z - (0.707107)) <= 0.01);

    p.x = 1;
    p.y = 0;
    p.z = 0;
    r.hori = 90;
    r.verti = 45;
    r.roll = 0;
    spew3d_math3d_rotate(&p, &r);
    assert(fabs(p.x - (0)) <= 0.01);
    assert(fabs(p.y - (0.707107)) <= 0.01);
    assert(fabs(p.z - (0.707107)) <= 0.01);
}
#endif  // #ifndef NDEBUG


#endif  // SPEW3D_IMPLEMENTATION

