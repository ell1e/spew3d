/* Copyright (c) 2020-2024, ellie/@ell1e & Spew3D Team (see AUTHORS.md).

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

/** Spew3D's default coordinate system for 2D
 *  =========================================
 *
 *  Positive angles describe CW (clockwise) rotations.
 *  X points right (on the screen), Y goes down (on the screen).
 */

#ifndef SPEW3D_MATH2D_H_
#define SPEW3D_MATH2D_H_

#include <math.h>

struct s3d_point {
    s3dnum_t x, y;
};
typedef struct s3d_point s3d_point;

#ifndef M_PI
#define M_PI (3.14159265358979323846)
#endif

S3DEXP void spew3d_math2d_rotate(
    s3d_point *p, s3dnum_t degree
);

static inline s3dnum_t spew3d_math2d_len(s3d_point *pos) {
    s3dnum_t result = round(sqrt(
        (pos->x * pos->x + pos->y * pos->y)
    ));
    return result;
}

static inline s3dnum_t spew3d_math2d_dist(
        s3d_point *pos1, s3d_point *pos2
        ) {
    s3dnum_t result = round(sqrt(
        ((pos1->x - pos2->x) * (pos1->x - pos2->x) +
        (pos1->y - pos2->y) * (pos1->y - pos2->y))
    ));
    return result;
}

S3DEXP int spew3d_math2d_lineintersect(
    s3d_point *line1_1, s3d_point *line1_2,
    s3d_point *line2_1, s3d_point *line2_2,
    s3d_point *out_intersect
);

S3DEXP void spew3d_math2d_nearestpointonsegment(
    s3d_point find_from_nearby,
    s3d_point line1, s3d_point line2,
    s3d_point *result
);

static inline void spew3d_math2d_rotatecenter(
        s3d_point *p, s3dnum_t degree,
        s3d_point center
        ) {
    p->x -= center.x;
    p->y -= center.y;
    spew3d_math2d_rotate(p, degree);
    p->x += center.x;
    p->y += center.y;
}

static inline s3dnum_t spew3d_math2d_angle(
        s3d_point *p
        ) {
    /// Return the angle of a point's origin to the point.
    /// Angles: (1.0, 0.0) returns 0 degrees angle,
    /// CW rotation increases angle. X is right, Y is down,
    /// (0.0, 1.0) returns 90 degrees angle.
    return (double)((atan2(p->y, p->x) / M_PI) * 180.0);
}

#endif  // SPEW3D_MATH2D_H_


