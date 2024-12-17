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

#include <assert.h>
#include <check.h>
#include <string.h>

#define SPEW3D_OPTION_DISABLE_SDL
#define SPEW3D_IMPLEMENTATION
#include "spew3d.h"

#include "testmain.h"

START_TEST (test_poly_rotate)
{
    s3d_pos v1;
    v1.x = 2;
    v1.y = 0;
    v1.z = 0;
    s3d_pos v2;
    v2.x = 0;
    v2.y = 1;
    v2.z = 0;
    s3d_pos v3;
    v3.x = 0;
    v3.y = 0;
    v3.z = 1;

    s3d_pos n;
    spew3d_math3d_polygon_normal(
        &v1, &v2, &v3, 0, &n
    );
    spew3d_math3d_normalize(&n);
    s3d_rotation nrot = spew3d_math3d_rotationfromto(
        NULL, &n
    );

    s3d_pos rcenter = {0};
    rcenter.x = (v1.x + v2.x + v3.x) / 3.0;
    rcenter.y = (v1.y + v2.y + v3.y) / 3.0;
    rcenter.z = (v1.z + v2.z + v3.z) / 3.0;
    s3d_rotation reverse1 = {0};
    reverse1.hori = -nrot.hori;
    s3d_rotation reverse2 = {0};
    reverse2.verti = -nrot.verti;
    spew3d_math3d_rotateat(&v1, &reverse1, &rcenter);
    spew3d_math3d_rotateat(&v2, &reverse1, &rcenter);
    spew3d_math3d_rotateat(&v3, &reverse1, &rcenter);
    spew3d_math3d_rotateat(&v1, &reverse2, &rcenter);
    spew3d_math3d_rotateat(&v2, &reverse2, &rcenter);
    spew3d_math3d_rotateat(&v3, &reverse2, &rcenter);

    s3d_pos nrotated = n;
    spew3d_math3d_rotate(&nrotated, &reverse1);
    spew3d_math3d_rotate(&nrotated, &reverse2);
    spew3d_math3d_normalize(&n);
    s3d_pos nrecalculated;
    spew3d_math3d_polygon_normal(
        &v1, &v2, &v3, 0, &nrecalculated
    );
    spew3d_math3d_normalize(&nrecalculated);

    assert(S3D_ABS(nrotated.x - (nrecalculated.x)) <= (0.01));
    assert(S3D_ABS(nrotated.y - (nrecalculated.y)) <= (0.01));
    assert(S3D_ABS(nrotated.z - (nrecalculated.z)) <= (0.01));
}
END_TEST

START_TEST (test_math_polygon_normal)
{
    s3d_pos v1 = {0};
    v1.z = 2;
    s3d_pos v2 = {0};
    v2.y = 2;
    s3d_pos v3 = {0};
    v3.z = 2;
    v3.y = 2;
    s3d_pos n;
    spew3d_math3d_polygon_normal(
        &v1, &v2, &v3, 1, &n
    );

    assert(S3D_ABS(n.x - (1)) <= (0.01));
    assert(S3D_ABS(n.y - (0)) <= (0.01));
    assert(S3D_ABS(n.z - (0)) <= (0.01));
    s3d_rotation r = spew3d_math3d_rotationfromto(
        NULL, &n
    );
    assert(S3D_ABS(r.hori - (0)) <= (0.01));

    memset(&v1, 0, sizeof(v1));
    v1.z = 2;
    memset(&v2, 0, sizeof(v2));
    v2.y = -2;
    memset(&v3, 0, sizeof(v3));
    v3.z = 2;
    v3.y = -2;
    spew3d_math3d_polygon_normal(
        &v1, &v2, &v3, 1, &n
    );

    assert(S3D_ABS(n.x - (-1)) <= (0.01));
    assert(S3D_ABS(n.y - (0)) <= (0.01));
    assert(S3D_ABS(n.z - (0)) <= (0.01));
    r = spew3d_math3d_rotationfromto(
        NULL, &n
    );
    assert(S3D_ABS(S3D_ABS(r.hori) - (180)) <= (0.01));

    memset(&v1, 0, sizeof(v1));
    v1.x = 2;
    memset(&v2, 0, sizeof(v2));
    v2.y = -2;
    memset(&v3, 0, sizeof(v3));
    v3.x = 2;
    v3.y = -2;
    spew3d_math3d_polygon_normal(
        &v1, &v2, &v3, 1, &n
    );

    assert(S3D_ABS(n.x - (0)) <= (0.01));
    assert(S3D_ABS(n.y - (0)) <= (0.01));
    assert(S3D_ABS(n.z - (1)) <= (0.01));
    r = spew3d_math3d_rotationfromto(
        NULL, &n
    );
    assert(S3D_ABS(r.verti - (90)) <= (0.01));

    memset(&v1, 0, sizeof(v1));
    v1.x = 2;
    memset(&v2, 0, sizeof(v2));
    v2.z = -2;
    memset(&v3, 0, sizeof(v3));
    v3.x = 2;
    v3.z = -2;
    spew3d_math3d_polygon_normal(
        &v1, &v2, &v3, 1, &n
    );

    assert(S3D_ABS(n.x - (0)) <= (0.01));
    assert(S3D_ABS(n.y - (-1)) <= (0.01));
    assert(S3D_ABS(n.z - (0)) <= (0.01));
    r = spew3d_math3d_rotationfromto(
        NULL, &n
    );
    assert(S3D_ABS(r.hori - (-90)) <= (0.01));

    memset(&v1, 0, sizeof(v1));
    v1.x = -2;
    memset(&v2, 0, sizeof(v2));
    v2.z = -2;
    memset(&v3, 0, sizeof(v3));
    v3.x = -2;
    v3.z = -2;
    spew3d_math3d_polygon_normal(
        &v1, &v2, &v3, 1, &n
    );

    assert(S3D_ABS(n.x - (0)) <= (0.01));
    assert(S3D_ABS(n.y - (1)) <= (0.01));
    assert(S3D_ABS(n.z - (0)) <= (0.01));
    r = spew3d_math3d_rotationfromto(
        NULL, &n
    );
    assert(S3D_ABS(r.hori - (90)) <= (0.01));
    assert(S3D_ABS(r.verti - (0)) <= (0.01));
    assert(S3D_ABS(r.roll - (0)) <= (0.01));
}
END_TEST

START_TEST (test_math_rotate_3d_2)
{
    s3d_rotation r;
    s3d_pos p;
    p.x = 1;
    p.y = 0;
    p.z = 0;
    r.hori = 90;
    r.verti = 45;
    r.roll = 0;
    spew3d_math3d_rotate(&p, &r);
    assert(S3D_ABS(p.x - (0)) <= (0.01));
    assert(S3D_ABS(p.y - (0.707)) <= (0.01));
    assert(S3D_ABS(p.z - (0.707)) <= (0.01));
    r = spew3d_math3d_rotationfromto(
        NULL, &p
    );
    assert(S3D_ABS(r.hori - (90)) <= (0.01));
    assert(S3D_ABS(r.verti - (45)) <= (0.01));
    assert(S3D_ABS(r.roll - (0)) <= (0.01));

    p.x = 4;
    p.y = -0.5;
    p.z = -0.5;
    r = spew3d_math3d_rotationfromto(
        NULL, &p
    );
    s3d_rotation reverse1 = {0};
    reverse1.hori = -r.hori;
    s3d_rotation reverse2 = {0};
    reverse2.verti = -r.verti;
    s3d_pos p2 = p;
    spew3d_math3d_rotate(&p2, &reverse1);
    spew3d_math3d_rotate(&p2, &reverse2);
    spew3d_math3d_normalize(&p2);

    assert(S3D_ABS(p2.x - (1)) <= (0.01));
    assert(S3D_ABS(p2.y - (0)) <= (0.01));
    assert(S3D_ABS(p2.z - (0)) <= (0.01));
}
END_TEST

START_TEST (test_math_rotate_3d)
{
    s3d_rotation r;
    s3d_pos p;
    p.x = 1;
    p.y = 0;
    p.z = 0;
    r.hori = 90;
    r.verti = 0;
    r.roll = 0;
    spew3d_math3d_rotate(&p, &r);
    assert(S3D_ABS(p.x - (0)) <= (0.01));
    assert(S3D_ABS(p.y - (1)) <= (0.01));
    assert(S3D_ABS(p.z - (0)) <= (0.01));
    r = spew3d_math3d_rotationfromto(
        NULL, &p
    );
    assert(S3D_ABS(r.hori - (90)) <= (0.01));
    assert(S3D_ABS(r.verti - (0)) <= (0.01));
    assert(S3D_ABS(r.roll - (0)) <= (0.01));

    p.x = 1;
    p.y = 0;
    p.z = 1;
    r.hori = 90;
    r.verti = 0;
    r.roll = 0;
    spew3d_math3d_rotate(&p, &r);
    assert(S3D_ABS(p.x - (0)) <= (0.01));
    assert(S3D_ABS(p.y - (1)) <= (0.01));
    assert(S3D_ABS(p.z - (1)) <= (0.01));

    p.x = 1;
    p.y = 1;
    p.z = 1;
    r.hori = 90;
    r.verti = 0;
    r.roll = 0;
    spew3d_math3d_rotate(&p, &r);
    assert(S3D_ABS(p.x - (-1)) <= (0.01));
    assert(S3D_ABS(p.y - (1)) <= (0.01));
    assert(S3D_ABS(p.z - (1)) <= (0.01));

    p.x = 1;
    p.y = 0;
    p.z = 1;
    r.hori = 90;
    r.verti = 0;
    r.roll = 0;
    spew3d_math3d_rotate(&p, &r);
    assert(S3D_ABS(p.x - (0)) <= (0.01));
    assert(S3D_ABS(p.y - (1)) <= (0.01));
    assert(S3D_ABS(p.z - (1)) <= (0.01));

    p.x = 1;
    p.y = 0;
    p.z = 0;
    r.hori = 45;
    r.verti = 45;
    r.roll = 0;
    spew3d_math3d_rotate(&p, &r);
    assert(S3D_ABS(p.x - (0.5)) <= (0.01));
    assert(S3D_ABS(p.y - (0.5)) <= (0.01));
    assert(S3D_ABS(p.z - (0.707107)) <= (0.01));

    p.x = 1;
    p.y = 1;
    p.z = 0;
    r.hori = 0;
    r.verti = 45;
    r.roll = 0;
    spew3d_math3d_rotate(&p, &r);
    assert(S3D_ABS(p.x - (0.707107)) <= (0.01));
    assert(S3D_ABS(p.y - (1)) <= (0.01));
    assert(S3D_ABS(p.z - (0.707107)) <= (0.01));

    p.x = 1;
    p.y = 0;
    p.z = 0;
    r.hori = 90;
    r.verti = 45;
    r.roll = 0;
    spew3d_math3d_rotate(&p, &r);
    assert(S3D_ABS(p.x - (0)) <= (0.01));
    assert(S3D_ABS(p.y - (0.707107)) <= (0.01));
    assert(S3D_ABS(p.z - (0.707107)) <= (0.01));
}
END_TEST

START_TEST (test_math_angle_rotate_2d)
{
    s3d_point p = {0};
    p.x = 1;
    p.y = 0;
    assert(S3D_ABS(spew3d_math2d_angle(&p) - (0.0))
           < 10);
    p.x = 0;
    p.y = 1;
    assert(S3D_ABS(spew3d_math2d_angle(&p) - (90.0))
           < 10);
    p.x = 1;
    p.y = -1;
    assert(S3D_ABS(spew3d_math2d_angle(&p) - (-45.0))
           < 10);

    p.x = 1;
    p.y = 0;
    spew3d_math2d_rotate(&p, (-90.0));
    assert(S3D_ABS(p.x - (0.0)) < (0.1));
    assert(S3D_ABS(p.y - (-1.0)) < (0.1));
}
END_TEST

START_TEST (test_math_fov_3d)
{
    s3dnum_t horifov, vertifov;
    spew3d_math3d_split_fovs_from_fov(
        80.0, 320, 240,
        &horifov, &vertifov
    );
    assert(horifov > 90 && horifov < 120);
    assert(vertifov > 45 && vertifov < 90);
}
END_TEST

START_TEST (test_math_angle_3d)
{
    s3d_pos p = {0};
    p.x = 1;
    s3d_pos p2 = {0};
    p2.y = 1;
    s3dnum_t rot = spew3d_math3d_anglefromto(
        &p, &p2
    );
    assert(fabs(rot - 90.0) <= 0.1);

    s3d_pos p3 = {0};
    p3.x = 1;
    s3d_pos p4 = {0};
    p4.x = 1;
    p4.y = 1;
    s3dnum_t rot2 = spew3d_math3d_anglefromto(
        &p3, &p4
    );
    assert(fabs(rot2 - 45.0) <= 0.1);
}
END_TEST

TESTS_MAIN(test_math_rotate_3d, test_math_angle_rotate_2d,
    test_math_angle_3d, test_math_polygon_normal,
    test_math_rotate_3d_2, test_poly_rotate)

