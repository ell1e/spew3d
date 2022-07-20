

#ifndef SPEW3D_GEOMETRY_H_
#define SPEW3D_GEOMETRY_H_

#include <stdint.h>

typedef struct spew3d_geometry {
    int32_t vertex_count;
    spew3d_pos *vertex;
    int32_t polygon_count;
    spew3d_pos *polygon_normal;
    int32_t *polygon_vertexindex;
    spew3d_point *polygon_texcoord;
    spew3d_texture_t *polygon_texture;

    int32_t owned_texture_count;
    spew3d_texture_t *owned_texture;
} spew3d_geometry;


spew3d_geometry *spew3d_geometry_Create();


int spew3d_geometry_AddCube(
    spew3d_geometry *geometry,
    double edge_width,
    spew3d_pos *offset,
    spew3d_rotation *rotation,
    spew3d_point *side_texcoord,
    spew3d_texture_t *side_texture
);

#endif  // SPEW3D_GEOMETRY_H_

