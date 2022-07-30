
#ifndef SPEW3D_MATH2D_H_
#define SPEW3D_MATH2D_H_

typedef struct spew3d_point {
    double x, y;
} spew3d_point;


static inline void spew3d_math2d_rotate(
        spew3d_point *p, double degree
        ) {
    /// Rotate a given point around its origin by the given degree.
    /// Positive angle gives CW (clockwise) rotation.
    /// X is right, Y is down.
    degree = (degree / 180.0) * M_PI;
    double newy = (p->y) * cos(degree) + (p->x) * sin(degree);
    double newx = (p->x) * cos(degree) - (p->y) * sin(degree);
    p->x = newx;
    p->y = newy;
}


static inline void spew3d_math2d_rotatecenter(
        spew3d_point *p, double degree,
        spew3d_point center
        ) {
    p->x -= center.x;
    p->y -= center.y;
    spew3d_math2d_rotate(p, degree);
    p->x += center.x;
    p->y += center.y;
}


static inline double spew3d_math2d_angle(
        spew3d_point *p
        ) {
    /// Return the angle of a point's origin to the point.
    /// Angles: (1.0, 0.0) returns 0 degrees angle,
    /// CW rotation increases angle. X is right, Y is down,
    /// (0.0, 1.0) returns 90 degrees angle.
    return ((atan2(p->y, p->x) / M_PI) * 180.0);
}

#endif  // SPEW3D_MATH2D_H_


