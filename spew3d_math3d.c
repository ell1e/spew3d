
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

