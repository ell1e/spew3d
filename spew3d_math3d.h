
#ifndef SPEW3D_MATH3D_H_
#define SPEW3D_MATH3D_H_

typedef struct spew3d_pos {
    double x, y, z;
} spew3d_pos;

typedef struct spew3d_rotation {
    double hori, verti, roll;
} spew3d_rotation;


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
