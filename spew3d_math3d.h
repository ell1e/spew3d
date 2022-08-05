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

#ifndef SPEW3D_MATH3D_H_
#define SPEW3D_MATH3D_H_

typedef struct spew3d_pos {
    double x, y, z;
} spew3d_pos;

typedef struct spew3d_rotation {
    double hori, verti, roll;
} spew3d_rotation;


static inline void spew3d_math3d_add(
        spew3d_pos *p, spew3d_pos *p2
        ) {
    p->x += p2->x;
    p->y += p2->y;
    p->z += p2->z;
}

static inline void spew3d_math3d_rotate(
        spew3d_pos *p, spew3d_rotation *r
        ) {
    /// Rotate a given pos around its origin by the given degrees.
    /// Positive angle gives CW (clockwise) rotation.
    /// X is forward (into screen), Y is left, Z is up.

    double roth = (r->hori / 180.0) * M_PI;
    double rotv = (r->verti / 180.0) * M_PI;
    double rotr = (r->roll / 180.0) * M_PI;
    double newx, newy, newz;

    // Roll angle:
    newy = (p->y) * cos(rotr) + (p->z) * sin(rotr);
    newz = (p->z) * cos(rotr) - (p->y) * sin(rotr);
    p->z = newz;
    p->y = newy;

    // Vertical angle:
    newz = (p->z) * cos(rotv) + (p->x) * sin(rotv);
    newx = (p->x) * cos(rotv) - (p->z) * sin(rotv);
    p->x = newx;
    p->z = newz;

    // Horizontal angle:
    newy = (p->y) * cos(roth) + (p->x) * sin(roth);
    newx = (p->x) * cos(roth) - (p->y) * sin(roth);
    p->x = newx;
    p->y = newy;
}

#endif  // SPEW3D_MATH3D_H_
