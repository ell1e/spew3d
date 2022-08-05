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

#ifndef SPEW3D_GEOMETRY_H_
#define SPEW3D_GEOMETRY_H_

#include <stdint.h>

typedef uint64_t spew3d_texture_t;
typedef struct spew3d_pos spew3d_pos;
typedef struct spew3d_point spew3d_point;
typedef struct spew3d_rotation spew3d_rotation;

typedef uint32_t spew3d_material_t;

#define SPEW3D_MATERIAL_TRANSPARENT33 ((uint32_t)1)
#define SPEW3D_MATERIAL_TRANSPARENT66 ((uint32_t)1 << 1)
#define SPEW3D_MATERIAL_OBJECTPASSABLE ((uint32_t)1 << 2)
#define SPEW3D_MATERIAL_BULLETSHOTPASSABLE ((uint32_t)1 << 3)
#define SPEW3D_MATERIAL_AISIGHTPASSABLE ((uint32_t)1 << 4)


typedef struct spew3d_geometry {
    int32_t vertex_count;
    spew3d_pos *vertex;
    int32_t polygon_count;
    spew3d_pos *polygon_normal;
    spew3d_material_t *polygon_material;
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
    spew3d_texture_t *side_texture,
    int *side_texture_owned
);

int spew3d_geometry_AddCubeSimple(
    spew3d_geometry *geometry,
    double edge_width,
    spew3d_texture_t texture,
    int texture_owned
);

void spew3d_geometry_Destroy(spew3d_geometry *geometry);


#endif  // SPEW3D_GEOMETRY_H_

