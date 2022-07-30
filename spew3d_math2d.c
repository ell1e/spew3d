
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

