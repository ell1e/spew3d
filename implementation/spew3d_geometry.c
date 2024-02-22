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

#ifdef SPEW3D_IMPLEMENTATION

#include <assert.h>
#ifndef SPEW3D_OPTION_DISABLE_SDL
#include <SDL2/SDL.h>
#endif
#include <string.h>

S3DHID int _internal_spew3d_geometry_AddVertexPolyAlloc(
        s3d_geometry *geometry,
        int add_vertex, int add_polygon
        ) {
    s3d_pos *new_vertex = realloc(
        geometry->vertex,
        sizeof(*new_vertex) *
        (geometry->vertex_count + add_vertex)
    );
    if (!new_vertex)
        return 0;
    geometry->vertex = new_vertex;

    s3d_pos *new_normal = realloc(
        geometry->polygon_normal,
        sizeof(*new_normal) *
        (geometry->polygon_count * 3 + add_polygon * 3)
    );
    if (!new_normal)
        return 0;
    geometry->polygon_normal = new_normal;

    uint32_t *new_vertexindex = realloc(
        geometry->polygon_vertexindex,
        sizeof(*new_vertexindex) *
        (geometry->polygon_count * 3 + add_polygon * 3)
    );
    if (!new_vertexindex)
        return 0;
    geometry->polygon_vertexindex = new_vertexindex;

    s3d_material_t *new_material = realloc(
        geometry->polygon_material,
        sizeof(*new_material) *
        (geometry->polygon_count * 3 + add_polygon * 3)
    );
    if (!new_material)
        return 0;
    geometry->polygon_material = new_material;

    s3d_color *new_polygon_vertexcolors = realloc(
        geometry->polygon_vertexcolors,
        sizeof(*new_polygon_vertexcolors) *
        (geometry->polygon_count * 3 + add_polygon * 3)
    );
    if (!new_polygon_vertexcolors)
        return 0;
    geometry->polygon_vertexcolors = new_polygon_vertexcolors;
    // Since we may not fill this one in, zero it out in advance:
    memset(&geometry->polygon_vertexcolors[
        geometry->polygon_count * 3], 0,
        sizeof(*new_polygon_vertexcolors) * (add_polygon * 3));

    s3d_pos *new_polygon_vertexnormals = realloc(
        geometry->polygon_vertexnormals,
        sizeof(*new_polygon_vertexnormals) *
        (geometry->polygon_count * 3 + add_polygon * 3)
    );
    if (!new_polygon_vertexnormals)
        return 0;
    geometry->polygon_vertexnormals = new_polygon_vertexnormals;
    // Since we may not fill this one in, zero it out in advance:
    memset(&geometry->polygon_vertexnormals[
        geometry->polygon_count * 3], 0,
        sizeof(*new_polygon_vertexnormals) * (add_polygon * 3));

    s3d_point *new_texcoord = realloc(
        geometry->polygon_texcoord,
        sizeof(*new_texcoord) *
        (geometry->polygon_count * 3 + add_polygon * 3)
    );
    if (!new_texcoord)
        return 0;
    geometry->polygon_texcoord = new_texcoord;

    s3d_texture_t *new_texture = realloc(
        geometry->polygon_texture,
        sizeof(*new_texture) *
        (geometry->polygon_count + add_polygon)
    );
    if (!new_vertexindex)
        return 0;
    geometry->polygon_texture = new_texture;
    return 1;
}

S3DEXP s3d_geometry *spew3d_geometry_Create() {
    s3d_geometry *geometry = malloc(sizeof(*geometry));
    if (!geometry)
        return NULL;

    memset(geometry, 0, sizeof(*geometry));
    return geometry;
}

S3DEXP int spew3d_geometry_AddCube(
        s3d_geometry *geometry,
        s3dnum_t edge_width,
        s3d_pos *offset,
        s3d_rotation *rotation,
        s3d_point *side_texcoord,
        s3d_texture_t *side_texture,
        int *side_texture_owned
        ) {
    if (!_internal_spew3d_geometry_AddVertexPolyAlloc(
            geometry, 8, 12
            )) {
        return 0;
    }
    geometry->vertex_count += 8;
    geometry->polygon_count += 12;

    const s3dnum_t halfedgewidth = (edge_width / 2);
    int viter = geometry->vertex_count - 8;
    assert(viter >= 0);
    const int viterstart = viter;
    while (viter < geometry->vertex_count) {
        const int relidx = (viter - viterstart);
        s3d_pos finaloffset = {0};
        finaloffset.z = (
            (relidx >= 4) ? -halfedgewidth : halfedgewidth
        );
        finaloffset.x = (
            (relidx == 0 || relidx == 3 || relidx == 4 ||
            relidx == 7) ? halfedgewidth : -halfedgewidth
        );
        finaloffset.y = (
            (relidx == 0 || relidx == 1 || relidx == 4 ||
            relidx == 5) ? halfedgewidth : -halfedgewidth
        );
        spew3d_math3d_rotate(&finaloffset, rotation);
        spew3d_math3d_add(&finaloffset, offset);
        geometry->vertex[viter] = finaloffset;
        viter++;
    }

    int poffset = geometry->polygon_count - 12;
    int32_t *vindex = geometry->polygon_vertexindex;
    s3d_point *txcoord = geometry->polygon_texcoord;

    // Forward / X+ side
    vindex[poffset * 3 + 0] = 3;  // top right
    vindex[poffset * 3 + 1] = 4;  // bottom left
    vindex[poffset * 3 + 2] = 0;  // top left
    txcoord[poffset * 3 + 0] = side_texcoord[1];
    txcoord[poffset * 3 + 1] = side_texcoord[3];
    txcoord[poffset * 3 + 2] = side_texcoord[0];
    geometry->polygon_texture[poffset] = side_texture[0];
    vindex[poffset * 3 + 3] = 3;  // top right
    vindex[poffset * 3 + 4] = 7;  // bottom right
    vindex[poffset * 3 + 5] = 4;  // bottom left
    txcoord[poffset * 3 + 3] = side_texcoord[1];
    txcoord[poffset * 3 + 4] = side_texcoord[2];
    txcoord[poffset * 3 + 5] = side_texcoord[3];
    geometry->polygon_texture[poffset + 1] = side_texture[0];
    poffset += 2;

    // Right / Y- side
    vindex[poffset * 3 + 0] = 2;  // top right
    vindex[poffset * 3 + 1] = 7;  // bottom left
    vindex[poffset * 3 + 2] = 3;  // top left
    txcoord[poffset * 3 + 0] = side_texcoord[4 + 1];
    txcoord[poffset * 3 + 1] = side_texcoord[4 + 3];
    txcoord[poffset * 3 + 2] = side_texcoord[4 + 0];
    geometry->polygon_texture[poffset] = side_texture[1];
    vindex[poffset * 3 + 3] = 2;  // top right
    vindex[poffset * 3 + 4] = 6;  // bottom right
    vindex[poffset * 3 + 5] = 7;  // bottom left
    txcoord[poffset * 3 + 3] = side_texcoord[4 + 1];
    txcoord[poffset * 3 + 4] = side_texcoord[4 + 2];
    txcoord[poffset * 3 + 5] = side_texcoord[4 + 3];
    geometry->polygon_texture[poffset + 1] = side_texture[1];
    poffset += 2;

    // Backward / X- side
    vindex[poffset * 3 + 0] = 1;  // top right
    vindex[poffset * 3 + 1] = 6;  // bottom left
    vindex[poffset * 3 + 2] = 2;  // top left
    txcoord[poffset * 3 + 0] = side_texcoord[8 + 1];
    txcoord[poffset * 3 + 1] = side_texcoord[8 + 3];
    txcoord[poffset * 3 + 2] = side_texcoord[8 + 0];
    geometry->polygon_texture[poffset] = side_texture[2];
    vindex[poffset * 3 + 3] = 1;  // top right
    vindex[poffset * 3 + 4] = 5;  // bottom right
    vindex[poffset * 3 + 5] = 6;  // bottom left
    txcoord[poffset * 3 + 3] = side_texcoord[8 + 1];
    txcoord[poffset * 3 + 4] = side_texcoord[8 + 2];
    txcoord[poffset * 3 + 5] = side_texcoord[8 + 3];
    geometry->polygon_texture[poffset + 1] = side_texture[2];
    poffset += 2;

    // Left / Y+ side
    vindex[poffset * 3 + 0] = 0;  // top right
    vindex[poffset * 3 + 1] = 5;  // bottom left
    vindex[poffset * 3 + 2] = 1;  // top left
    txcoord[poffset * 3 + 0] = side_texcoord[12 + 1];
    txcoord[poffset * 3 + 1] = side_texcoord[12 + 3];
    txcoord[poffset * 3 + 2] = side_texcoord[12 + 0];
    geometry->polygon_texture[poffset] = side_texture[3];
    vindex[poffset * 3 + 3] = 0;  // top right
    vindex[poffset * 3 + 4] = 4;  // bottom right
    vindex[poffset * 3 + 5] = 5;  // bottom left
    txcoord[poffset * 3 + 3] = side_texcoord[12 + 1];
    txcoord[poffset * 3 + 4] = side_texcoord[12 + 2];
    txcoord[poffset * 3 + 5] = side_texcoord[12 + 3];
    geometry->polygon_texture[poffset + 1] = side_texture[3];
    poffset += 2;

    // Bottom / Z- side
    vindex[poffset * 3 + 0] = 5;  // top right
    vindex[poffset * 3 + 1] = 7;  // bottom left
    vindex[poffset * 3 + 2] = 6;  // top left
    txcoord[poffset * 3 + 0] = side_texcoord[16 + 1];
    txcoord[poffset * 3 + 1] = side_texcoord[16 + 3];
    txcoord[poffset * 3 + 2] = side_texcoord[16 + 0];
    geometry->polygon_texture[poffset] = side_texture[4];
    vindex[poffset * 3 + 3] = 5;  // top right
    vindex[poffset * 3 + 4] = 4;  // bottom right
    vindex[poffset * 3 + 5] = 7;  // bottom left
    txcoord[poffset * 3 + 3] = side_texcoord[16 + 1];
    txcoord[poffset * 3 + 4] = side_texcoord[16 + 2];
    txcoord[poffset * 3 + 5] = side_texcoord[16 + 3];
    geometry->polygon_texture[poffset + 1] = side_texture[4];
    poffset += 2;

    // Top / Z+ side
    vindex[poffset * 3 + 0] = 2;  // top right
    vindex[poffset * 3 + 1] = 0;  // bottom left
    vindex[poffset * 3 + 2] = 1;  // top left
    txcoord[poffset * 3 + 0] = side_texcoord[20 + 1];
    txcoord[poffset * 3 + 1] = side_texcoord[20 + 3];
    txcoord[poffset * 3 + 2] = side_texcoord[20 + 0];
    geometry->polygon_texture[poffset] = side_texture[5];
    vindex[poffset * 3 + 3] = 2;  // top right
    vindex[poffset * 3 + 4] = 3;  // bottom right
    vindex[poffset * 3 + 5] = 0;  // bottom left
    txcoord[poffset * 3 + 3] = side_texcoord[20 + 1];
    txcoord[poffset * 3 + 4] = side_texcoord[20 + 2];
    txcoord[poffset * 3 + 5] = side_texcoord[20 + 3];
    geometry->polygon_texture[poffset + 1] = side_texture[5];
    poffset += 2;

    return 1;
}

S3DEXP int spew3d_geometry_AddPlaneSimple(
        s3d_geometry *geometry,
        s3dnum_t plane_width, s3dnum_t plane_height,
        int faces_upward, s3d_texture_t texture,
        int texture_owned) {
    s3d_point coords[4];

    // Top left:
    coords[0].x = 0;
    coords[0].y = 0;
    // Top right:
    coords[1].x = 1;
    coords[1].y = 0;
    // Bottom right:
    coords[2].x = 1;
    coords[2].y = 1;
    // Bottom left:
    coords[3].x = 0;
    coords[3].y = 1;

    s3d_pos offset = {0};
    s3d_rotation rotation = {0};
    if (!faces_upward)
        rotation.verti = 90;

    return spew3d_geometry_AddPlane(
        geometry, plane_width, plane_height,
        &offset, &rotation,
        coords, texture, texture_owned
    );
}

S3DEXP int spew3d_geometry_AddPlane(
        s3d_geometry *geometry,
        s3dnum_t plane_width, s3dnum_t plane_height,
        s3d_pos *offset,
        s3d_rotation *rotation,
        s3d_point *side_texcoord,
        s3d_texture_t texture,
        int texture_owned
        ) {
    if (!_internal_spew3d_geometry_AddVertexPolyAlloc(
            geometry, 4, 2
            )) {
        return 0;
    }
    geometry->vertex_count += 4;
    geometry->polygon_count += 2;

    const s3dnum_t halfplanewidth = (plane_width / 2);
    const s3dnum_t halfplaneheight = (plane_height / 2);
    int viter = geometry->vertex_count - 4;
    assert(viter >= 0);
    const int viterstart = viter;
    while (viter < geometry->vertex_count) {
        const int relidx = (viter - viterstart);
        s3d_pos finaloffset = {0};
        finaloffset.z = 0;
        finaloffset.x = (
            (relidx == 0 || relidx == 3 || relidx == 4 ||
            relidx == 7) ? halfplaneheight : -halfplaneheight
        );
        finaloffset.y = (
            (relidx == 0 || relidx == 1 || relidx == 4 ||
            relidx == 5) ? halfplanewidth : -halfplanewidth
        );
        spew3d_math3d_rotate(&finaloffset, rotation);
        spew3d_math3d_add(&finaloffset, offset);
        geometry->vertex[viter] = finaloffset;
        viter++;
    }

    int poffset = geometry->polygon_count - 2;
    int32_t *vindex = geometry->polygon_vertexindex;
    s3d_point *txcoord = geometry->polygon_texcoord;

    // Forward / X+ side
    vindex[poffset * 3 + 0] = 2;  // top right
    vindex[poffset * 3 + 1] = 0;  // bottom left
    vindex[poffset * 3 + 2] = 1;  // top left
    txcoord[poffset * 3 + 0] = side_texcoord[1];
    txcoord[poffset * 3 + 1] = side_texcoord[3];
    txcoord[poffset * 3 + 2] = side_texcoord[0];
    geometry->polygon_texture[poffset] = texture;
    vindex[poffset * 3 + 3] = 2;  // top right
    vindex[poffset * 3 + 4] = 3;  // bottom right
    vindex[poffset * 3 + 5] = 0;  // bottom left
    txcoord[poffset * 3 + 3] = side_texcoord[1];
    txcoord[poffset * 3 + 4] = side_texcoord[2];
    txcoord[poffset * 3 + 5] = side_texcoord[3];
    geometry->polygon_texture[poffset + 1] = texture;
    poffset += 2;

    return 1;
}

S3DEXP int spew3d_geometry_AddCubeSimple(
        s3d_geometry *geometry,
        s3dnum_t edge_width,
        s3d_texture_t texture,
        int texture_owned
        ) {
    s3d_point coords[4 * 6];
    int texture_owned_flag[6];
    s3d_texture_t textures[6];
    int i = 0;
    while (i < 6) {
        texture_owned_flag[i] = (texture_owned != 0);
        textures[i] = texture;

        // Top left:
        coords[i * 4 + 0].x = 0;
        coords[i * 4 + 0].y = 0;
        // Top right:
        coords[i * 4 + 1].x = 1;
        coords[i * 4 + 1].y = 0;
        // Bottom right:
        coords[i * 4 + 2].x = 1;
        coords[i * 4 + 2].y = 1;
        // Bottom left:
        coords[i * 4 + 3].x = 0;
        coords[i * 4 + 3].y = 1;

        i++;
    }
    s3d_pos offset = {0};
    s3d_rotation rotation = {0};

    return spew3d_geometry_AddCube(
        geometry, edge_width, &offset, &rotation,
        coords, textures, texture_owned_flag
    );
}

extern s3d_mutex *geometrydestroylist_m;
extern s3d_geometry **geometrydestroylist;
extern uint32_t geometrydestroylist_fill;
extern uint32_t geometrydestroylist_alloc;

S3DHID void _spew3d_geometry_ActuallyDestroy(s3d_geometry *geometry) {
    if (!geometry)
        return;
    uint32_t i = 0;
    while (i < geometry->owned_texture_count) {
        spew3d_texture_Destroy(geometry->owned_texture[i]);
        i++;
    }
    free(geometry->vertex);
    free(geometry->polygon_normal);
    free(geometry->polygon_material);
    free(geometry->polygon_vertexindex);
    free(geometry->polygon_texcoord);
    free(geometry->polygon_texture);
    free(geometry->polygon_vertexcolors);
}

S3DEXP void spew3d_geometry_Destroy(s3d_geometry *geometry) {
    if (!geometry)
        return;
    geometry->wasdeleted = 1;
    int result = spew3d_Deletion_Queue(DELETION_GEOM, geometry);
}

S3DEXP int spew3d_geometry_Transform(
        s3d_geometry *geometry,
        s3d_pos *model_pos,
        s3d_rotation *model_rotation,
        s3d_transform3d_cam_info *cam_info,
        s3d_geometryrenderlightinfo *render_light_info,
        s3d_renderpolygon **render_queue,
        uint32_t *render_fill, uint32_t *render_alloc
        ) {
    assert(render_light_info->dynlight_mode !=
        DLRD_INVALID);
    if (geometry->polygon_count == 0)
        return 0;

    s3d_renderpolygon *rqueue = *render_queue;
    uint32_t ralloc = *render_alloc;
    uint32_t rfill = *render_fill;
    if (rfill + geometry->polygon_count > ralloc) {
        int newalloc = (
            rfill + geometry->polygon_count + 1 + 6
        ) * 2;
        s3d_renderpolygon *newqueue = realloc(
            rqueue, sizeof(*newqueue) * newalloc
        );
        if (!newqueue)
            return 0;
        rqueue = newqueue;
        ralloc = newalloc;
        *render_queue = rqueue;
        *render_alloc = ralloc;
    }
    assert(ralloc > 0 && rqueue != NULL);
    const int have_vertex_normals = (
        geometry->per_vertex_normals_computed
    );
    double multiplier_vertex_light = (
        geometry->per_polygon_emit_computed ? 1.0 : 0.0
    );
    s3d_color scene_ambient = render_light_info->ambient_emit;
    if (render_light_info->dynlight_mode == DLRD_UNLIT) {
        if (geometry->per_polygon_emit_computed) {
            // Use mesh light instead:
            scene_ambient.red = 0.0;
            scene_ambient.green = 0.0;
            scene_ambient.blue = 0.0;
        } else {
            // Just go full bright:
            scene_ambient.red = 1.0;
            scene_ambient.green = 1.0;
            scene_ambient.blue = 1.0;
        }
    }

    s3d_pos effective_model_pos = {0};
    if (model_pos != NULL)
        memcpy(&effective_model_pos, model_pos,
            sizeof(*model_pos));
    s3d_rotation effective_model_rot = {0};
    if (model_rotation != NULL)
        memcpy(&effective_model_rot, model_rotation,
            sizeof(*model_rotation));

    uint32_t rfill_old = rfill;
    uint32_t ioffset = 0;
    uint32_t i = 0;
    while (i < geometry->polygon_count) {
        assert(rfill >= 0 && rfill < ralloc);
        assert(ioffset <= geometry->polygon_count * 3 - 3);
        rqueue[rfill].polygon_texture = (
            geometry->polygon_texture[i]
        );
        s3d_pos vertex_positions[3];

        // First vertex:
        #if defined(DEBUG_SPEW3D_TRANSFORM3D)
        printf("spew3d_geometry.c: debug: geom %p "
            "vertex #%d input world x,y,z %f,%f,%f\n",
            geometry, ioffset,
            (double)geometry->vertex[
                geometry->polygon_vertexindex[ioffset]
            ].x,
            (double)geometry->vertex[
                geometry->polygon_vertexindex[ioffset]
            ].y,
            (double)geometry->vertex[
                geometry->polygon_vertexindex[ioffset]
            ].z);
        #endif
        spew3d_math3d_transform3d(
            geometry->vertex[
                geometry->polygon_vertexindex[ioffset]
            ],
            cam_info, effective_model_pos,
            effective_model_rot,
            &rqueue[rfill].vertex_pos_pixels[0],
            &rqueue[rfill].vertex_pos[0]
        );
        #if defined(DEBUG_SPEW3D_TRANSFORM3D)
        printf("spew3d_geometry.c: debug: geom %p "
            "vertex #%d output world x,y,z %f,%f,%f\n",
            geometry, ioffset,
            (double)rqueue[rfill].vertex_pos_pixels[0].x,
            (double)rqueue[rfill].vertex_pos_pixels[0].y,
            (double)rqueue[rfill].vertex_pos_pixels[0].z);
        #endif
        rqueue[rfill].vertex_texcoord[0] = (
            geometry->polygon_texcoord[ioffset]
        );
        rqueue[rfill].vertex_emit[0] = (
            geometry->polygon_vertexcolors[ioffset]
        );
        rqueue[rfill].vertex_emit[0].red = fmax((
            rqueue[rfill].vertex_emit[0].red *
            multiplier_vertex_light), scene_ambient.red
        );
        rqueue[rfill].vertex_emit[0].green = fmax((
            rqueue[rfill].vertex_emit[0].green *
            multiplier_vertex_light), scene_ambient.green
        );
        rqueue[rfill].vertex_emit[0].blue = fmax((
            rqueue[rfill].vertex_emit[0].blue *
            multiplier_vertex_light), scene_ambient.blue
        );
        ioffset++;

        // Second vertex:
        #if defined(DEBUG_SPEW3D_TRANSFORM3D)
        printf("spew3d_geometry.c: debug: geom %p "
            "vertex #%d input world x,y,z %f,%f,%f\n",
            geometry, ioffset,
            (double)geometry->vertex[
                geometry->polygon_vertexindex[ioffset]
            ].x,
            (double)geometry->vertex[
                geometry->polygon_vertexindex[ioffset]
            ].y,
            (double)geometry->vertex[
                geometry->polygon_vertexindex[ioffset]
            ].z);
        #endif
        spew3d_math3d_transform3d(
            geometry->vertex[
                geometry->polygon_vertexindex[ioffset]
            ],
            cam_info, effective_model_pos,
            effective_model_rot,
            &rqueue[rfill].vertex_pos_pixels[1],
            &rqueue[rfill].vertex_pos[1]
        );
        #if defined(DEBUG_SPEW3D_TRANSFORM3D)
        printf("spew3d_geometry.c: debug: geom %p "
            "vertex #%d output world x,y,z %f,%f,%f\n",
            geometry, ioffset,
            (double)rqueue[rfill].vertex_pos_pixels[1].x,
            (double)rqueue[rfill].vertex_pos_pixels[1].y,
            (double)rqueue[rfill].vertex_pos_pixels[1].z);
        #endif
        rqueue[rfill].vertex_texcoord[1] = (
            geometry->polygon_texcoord[ioffset]
        );
        rqueue[rfill].vertex_emit[1] = (
            geometry->polygon_vertexcolors[ioffset]
        );
        rqueue[rfill].vertex_emit[1].red = fmax((
            rqueue[rfill].vertex_emit[1].red *
            multiplier_vertex_light), scene_ambient.red
        );
        rqueue[rfill].vertex_emit[1].green = fmax((
            rqueue[rfill].vertex_emit[1].green *
            multiplier_vertex_light), scene_ambient.green
        );
        rqueue[rfill].vertex_emit[1].blue = fmax((
            rqueue[rfill].vertex_emit[1].blue *
            multiplier_vertex_light), scene_ambient.blue
        );
        ioffset++;

        // Third vertex:
        #if defined(DEBUG_SPEW3D_TRANSFORM3D)
        printf("spew3d_geometry.c: debug: geom %p "
            "vertex #%d input world x,y,z %f,%f,%f\n",
            geometry, ioffset,
            (double)geometry->vertex[
                geometry->polygon_vertexindex[ioffset]
            ].x,
            (double)geometry->vertex[
                geometry->polygon_vertexindex[ioffset]
            ].y,
            (double)geometry->vertex[
                geometry->polygon_vertexindex[ioffset]
            ].z);
        #endif
        spew3d_math3d_transform3d(
            geometry->vertex[
                geometry->polygon_vertexindex[ioffset]
            ],
            cam_info, effective_model_pos,
            effective_model_rot,
            &rqueue[rfill].vertex_pos_pixels[2],
            &rqueue[rfill].vertex_pos[2]
        );
        #if defined(DEBUG_SPEW3D_TRANSFORM3D)
        printf("spew3d_geometry.c: debug: geom %p "
            "vertex #%d output world x,y,z %f,%f,%f\n",
            geometry, ioffset,
            (double)rqueue[rfill].vertex_pos_pixels[2].x,
            (double)rqueue[rfill].vertex_pos_pixels[2].y,
            (double)rqueue[rfill].vertex_pos_pixels[2].z);
        #endif
        rqueue[rfill].vertex_texcoord[2] = (
            geometry->polygon_texcoord[ioffset]
        );
        rqueue[rfill].vertex_emit[2] = (
            geometry->polygon_vertexcolors[ioffset]
        );
        rqueue[rfill].vertex_emit[2].red = fmax((
            rqueue[rfill].vertex_emit[2].red *
            multiplier_vertex_light), scene_ambient.red
        );
        rqueue[rfill].vertex_emit[2].green = fmax((
            rqueue[rfill].vertex_emit[2].green *
            multiplier_vertex_light), scene_ambient.green
        );
        rqueue[rfill].vertex_emit[2].blue = fmax((
            rqueue[rfill].vertex_emit[2].blue *
            multiplier_vertex_light), scene_ambient.blue
        );
        ioffset++;

        // Compute center:
        s3d_pos center;
        center.x = (rqueue[rfill].vertex_pos[0].x +
            rqueue[rfill].vertex_pos[1].x +
            rqueue[rfill].vertex_pos[2].x) / 3.0;
        center.y = (rqueue[rfill].vertex_pos[0].y +
            rqueue[rfill].vertex_pos[1].y +
            rqueue[rfill].vertex_pos[2].y) / 3.0;
        center.z = (rqueue[rfill].vertex_pos[0].z +
            rqueue[rfill].vertex_pos[1].z +
            rqueue[rfill].vertex_pos[2].z) / 3.0;
        rqueue[rfill].center = center;
        rqueue[rfill].min_depth = fmin(fmin(
            rqueue[rfill].vertex_pos[0].x,
            rqueue[rfill].vertex_pos[1].x),
            rqueue[rfill].vertex_pos[2].x);
        rqueue[rfill].max_depth = fmax(fmax(
            rqueue[rfill].vertex_pos[0].x,
            rqueue[rfill].vertex_pos[1].x),
            rqueue[rfill].vertex_pos[2].x);

        // If the polygon as a whole isn't in front of the camera, clip it:
        if (rqueue[rfill].max_depth < 0 || center.x < 0) {
            i++;
            // No rfill++ here since we're abandoning this slot.
            continue;
        }

        // Misc:
        rqueue[rfill].polygon_material = (
            geometry->polygon_material[i]
        );
        memset(&rqueue[rfill].vertex_normal[0], 0,
            sizeof(rqueue[rfill].vertex_normal[0]) * 3);

        rfill++;
        i++;
    }
    if (render_light_info->dynlight_mode >= DLRD_LIT_FULLY) {
        ioffset = 0;
        i = 0;
        while (i < geometry->polygon_count) {
            if (have_vertex_normals) {
                /// FIXME
                assert(0);
            } else {
                rqueue[rfill_old + i].vertex_normal[0] =
                    geometry->polygon_normal[i];
                rqueue[rfill_old + i].vertex_normal[1] =
                    geometry->polygon_normal[i];
                rqueue[rfill_old + i].vertex_normal[2] =
                    geometry->polygon_normal[i];
            }
            // FIXME: also compute the actual light here.
            i++;
        }
    }
    *render_fill = rfill;
    return 1;
}

#endif  // SPEW3D_IMPLEMENTATION

