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
#endif  // not(NDEBUG)


#endif  // SPEW3D_IMPLEMENTATION

