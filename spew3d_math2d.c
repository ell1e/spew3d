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
#include <string.h>


#ifndef NDEBUG
static void __attribute__((constructor))
        _spew3d_math2dtest() {
    spew3d_point p = {0};
    p.x = 1;
    p.y = 0;
    assert(fabs(spew3d_math2d_angle(&p) - 0.0) < 0.1);
    p.x = 0;
    p.y = 1;
    assert(fabs(spew3d_math2d_angle(&p) - 90.0) < 0.1);
    p.x = 1;
    p.y = -1;
    assert(fabs(spew3d_math2d_angle(&p) - (-45.0)) < 0.1);

    p.x = 1;
    p.y = 0;
    spew3d_math2d_rotate(&p, -90);
    assert(fabs(p.x - 0.0) < 0.1);
    assert(fabs(p.y - (-1.0)) < 0.1);
}
#endif  // not(NDEBUG)


#endif  // SPEW3D_IMPLEMENTATION

