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

#if defined(SPEW3D_IMPLEMENTATION) && \
    SPEW3D_IMPLEMENTATION != 0

#include <assert.h>
#include <inttypes.h>
#include <math.h>
#include <string.h>

S3DEXP int spew3d_math2d_lineintersect(
        s3d_point *line1_1, s3d_point *line1_2,
        s3d_point *line2_1, s3d_point *line2_2,
        s3d_point *out_intersect
        ) {
    s3dnum_t l1x1 = line1_1->x;
    s3dnum_t l1y1 = line1_1->y;
    s3dnum_t l1x2 = line1_2->x;
    s3dnum_t l1y2 = line1_2->y;
    s3dnum_t l2x1 = line2_1->x;
    s3dnum_t l2y1 = line2_1->y;
    s3dnum_t l2x2 = line2_2->x;
    s3dnum_t l2y2 = line2_2->y;
    // If the two line segments intersect,
    // returns 1 and sets ix, iy coordinates of intersection.
    // Otherwise, returns 0.

    // Based on https://github.com/psalaets/line-intersect,
    // also see 3RDPARTYCREDITS.md:
    s3dnum_t dval = ((l2y2 - l2y1) * (l1x2 - l1x1)) -
        ((l2x2 - l2x1) * (l1y2 - l1y1));
    s3dnum_t valA = ((l2x2 - l2x1) * (l1y1 - l2y1)) -
        ((l2y2 - l2y1) * (l1x1 - l2x1));
    s3dnum_t valB = ((l1x2 - l1x1) * (l1y1 - l2y1)) -
        ((l1y2 - l1y1) * (l1x1 - l2x1));
    if (dval == 0) {
        return 0;
    }
    s3dnum_t slopeA = valA / (s3dnum_t)dval;
    s3dnum_t slopeB = valB / (s3dnum_t)dval;
    if (slopeA >= 0 && slopeA <= 1 &&
            slopeB >= 0 && slopeB <= 1) {
        out_intersect->x = (l1x1 + (slopeA * (l1x2 - l1x1)));
        out_intersect->y = (l1y1 + (slopeA * (l1y2 - l1y1)));
        return 1;
    }
    return 0;
}

S3DEXP void spew3d_math2d_rotate(
        s3d_point *p, s3dnum_t degree
        ) {
    /// Rotate a given point around its origin by the given degree.
    /// Positive angle gives CW (clockwise) rotation.
    /// X is right, Y is down.
    double d_degree = ((double)degree * M_PI) / 180.0;
    double py_d = p->y;
    double px_d = p->x;
    double newy = py_d * cos(d_degree) + px_d * sin(d_degree);
    double newx = px_d * cos(d_degree) - py_d * sin(d_degree);
    p->x = newx;
    p->y = newy;
}

S3DEXP void spew3d_math2d_nearestpointonsegment(
        s3d_point find_from_nearby,
        s3d_point line1, s3d_point line2,
        s3d_point *result
        ) {
    if (line1.x == line2.x && line1.y == line2.y) {
        result->x = line1.x;
        result->y = line1.y;
        return;
    }
    // Rotate things to be seen locally to the line:
    s3d_point linedir;
    linedir.x = line2.x - line1.x;
    linedir.y = line2.y - line1.y;
    s3dnum_t angle_segment = spew3d_math2d_angle(
        &linedir
    );
    int64_t len_segment = spew3d_math2d_len(&linedir);
    assert(len_segment >= 0);
    find_from_nearby.x -= line1.x;
    find_from_nearby.y -= line1.y;
    spew3d_math2d_rotate(&find_from_nearby, -angle_segment);
    if (find_from_nearby.x < 0) {
        result->x = line1.x;
        result->y = line1.y;
    } else if (find_from_nearby.x > len_segment) {
        result->x = line2.x;
        result->y = line2.y;
    } else {
        // Rotate result back into world space:
        result->x = find_from_nearby.x;
        result->y = 0;
        spew3d_math2d_rotate(&find_from_nearby, -angle_segment);
        spew3d_math2d_rotate(result, angle_segment);
        result->x += line1.x;
        result->y += line1.y;
    }
}

#endif  // SPEW3D_IMPLEMENTATION

