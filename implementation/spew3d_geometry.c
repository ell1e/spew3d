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
        (geometry->polygon_count + add_polygon * 3)
    );
    if (!new_normal)
        return 0;
    geometry->polygon_normal = new_normal;

    int32_t *new_vertexindex = realloc(
        geometry->polygon_vertexindex,
        sizeof(*new_vertexindex) *
        (geometry->polygon_count + add_polygon * 3)
    );
    if (!new_vertexindex)
        return 0;
    geometry->polygon_vertexindex = new_vertexindex;

    s3d_material_t *new_material = realloc(
        geometry->polygon_material,
        sizeof(*new_material) *
        (geometry->polygon_count + add_polygon)
    );
    if (!new_material)
        return 0;
    geometry->polygon_material = new_material;

    s3d_color *new_polygon_vertexcolors = realloc(
        geometry->polygon_vertexcolors,
        sizeof(*new_polygon_vertexcolors) *
        (geometry->polygon_count + add_polygon * 3)
    );
    if (!new_polygon_vertexcolors)
        return 0;
    geometry->polygon_vertexcolors = new_polygon_vertexcolors;
    // Since we may not fill this one in, zero it out in advance:
    memset(&geometry->polygon_vertexcolors[
        geometry->polygon_count * 3], 0,
        sizeof(*new_polygon_vertexcolors) * (add_polygon * 3));

    s3d_color *new_polygon_vertexnormals = realloc(
        geometry->polygon_vertexnormals,
        sizeof(*new_polygon_vertexnormals) *
        (geometry->polygon_count + add_polygon * 3)
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
        (geometry->polygon_count + add_polygon * 3)
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
    vindex[poffset * 3 + 0] = 3;  // bottom left
    vindex[poffset * 3 + 1] = 4;  // top right
    vindex[poffset * 3 + 2] = 0;  // bottom right
    txcoord[poffset * 3 + 0] = side_texcoord[3];
    txcoord[poffset * 3 + 1] = side_texcoord[1];
    txcoord[poffset * 3 + 2] = side_texcoord[2];
    vindex[poffset * 3 + 3] = 3;  // bottom left
    vindex[poffset * 3 + 4] = 7;  // top left
    vindex[poffset * 3 + 5] = 4;  // top right
    txcoord[poffset * 3 + 0] = side_texcoord[3];
    txcoord[poffset * 3 + 1] = side_texcoord[0];
    txcoord[poffset * 3 + 2] = side_texcoord[1];
    poffset += 2;

    // Right / Y- side
    vindex[poffset * 3 + 0] = 2;  // bottom left
    vindex[poffset * 3 + 1] = 7;  // top right
    vindex[poffset * 3 + 2] = 3;  // bottom right
    txcoord[poffset * 3 + 0] = side_texcoord[4 + 3];
    txcoord[poffset * 3 + 1] = side_texcoord[4 + 1];
    txcoord[poffset * 3 + 2] = side_texcoord[4 + 2];
    vindex[poffset * 3 + 3] = 2;  // bottom left
    vindex[poffset * 3 + 4] = 6;  // top left
    vindex[poffset * 3 + 5] = 7;  // top right
    txcoord[poffset * 3 + 0] = side_texcoord[4 + 3];
    txcoord[poffset * 3 + 1] = side_texcoord[4 + 0];
    txcoord[poffset * 3 + 2] = side_texcoord[4 + 1];
    poffset += 2;

    // Backward / X- side
    vindex[poffset * 3 + 0] = 1;  // bottom left
    vindex[poffset * 3 + 1] = 6;  // top right
    vindex[poffset * 3 + 2] = 2;  // bottom right
    txcoord[poffset * 3 + 0] = side_texcoord[8 + 3];
    txcoord[poffset * 3 + 1] = side_texcoord[8 + 1];
    txcoord[poffset * 3 + 2] = side_texcoord[8 + 2];
    vindex[poffset * 3 + 3] = 1;  // bottom left
    vindex[poffset * 3 + 4] = 5;  // top left
    vindex[poffset * 3 + 5] = 6;  // top right
    txcoord[poffset * 3 + 0] = side_texcoord[8 + 3];
    txcoord[poffset * 3 + 1] = side_texcoord[8 + 0];
    txcoord[poffset * 3 + 2] = side_texcoord[8 + 1];
    poffset += 2;

    // Left / Y+ side
    vindex[poffset * 3 + 0] = 0;  // bottom left
    vindex[poffset * 3 + 1] = 5;  // top right
    vindex[poffset * 3 + 2] = 1;  // bottom right
    txcoord[poffset * 3 + 0] = side_texcoord[12 + 3];
    txcoord[poffset * 3 + 1] = side_texcoord[12 + 1];
    txcoord[poffset * 3 + 2] = side_texcoord[12 + 2];
    vindex[poffset * 3 + 3] = 0;  // bottom left
    vindex[poffset * 3 + 4] = 4;  // top left
    vindex[poffset * 3 + 5] = 5;  // top right
    txcoord[poffset * 3 + 0] = side_texcoord[12 + 3];
    txcoord[poffset * 3 + 1] = side_texcoord[12 + 0];
    txcoord[poffset * 3 + 2] = side_texcoord[12 + 1];
    poffset += 2;

    // Top / Z+ side
    vindex[poffset * 3 + 0] = 5;  // bottom left
    vindex[poffset * 3 + 1] = 7;  // top right
    vindex[poffset * 3 + 2] = 6;  // bottom right
    txcoord[poffset * 3 + 0] = side_texcoord[16 + 3];
    txcoord[poffset * 3 + 1] = side_texcoord[16 + 1];
    txcoord[poffset * 3 + 2] = side_texcoord[16 + 2];
    vindex[poffset * 3 + 3] = 5;  // bottom left
    vindex[poffset * 3 + 4] = 4;  // top left
    vindex[poffset * 3 + 5] = 7;  // top right
    txcoord[poffset * 3 + 0] = side_texcoord[16 + 3];
    txcoord[poffset * 3 + 1] = side_texcoord[16 + 0];
    txcoord[poffset * 3 + 2] = side_texcoord[16 + 1];
    poffset += 2;

    // Down / Z- side
    vindex[poffset * 3 + 0] = 2;  // bottom left
    vindex[poffset * 3 + 1] = 0;  // top right
    vindex[poffset * 3 + 2] = 1;  // bottom right
    txcoord[poffset * 3 + 0] = side_texcoord[20 + 3];
    txcoord[poffset * 3 + 1] = side_texcoord[20 + 1];
    txcoord[poffset * 3 + 2] = side_texcoord[20 + 2];
    vindex[poffset * 3 + 3] = 2;  // bottom left
    vindex[poffset * 3 + 4] = 3;  // top left
    vindex[poffset * 3 + 5] = 0;  // top right
    txcoord[poffset * 3 + 0] = side_texcoord[20 + 3];
    txcoord[poffset * 3 + 1] = side_texcoord[20 + 0];
    txcoord[poffset * 3 + 2] = side_texcoord[20 + 1];
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
    mutex_Lock(geometrydestroylist_m);
    if (geometrydestroylist_fill + 1 >
            geometrydestroylist_alloc) {
        uint32_t new_alloc = (
            geometrydestroylist_fill + 1 + 32
        ) * 2;
        s3d_geometry **newlist = realloc(
            geometrydestroylist,
            sizeof(*geometrydestroylist) * new_alloc
        );
        if (!newlist) {
            fprintf(stderr, "spew3d_geometry.c: error: "
                "Failed to allocate deletion entry for "
                "geometry entry. Will leak memory.");
            mutex_Release(geometrydestroylist_m);
            return;
        }
        geometrydestroylist = newlist;
        geometrydestroylist_alloc = new_alloc;
    }
    geometrydestroylist[geometrydestroylist_fill] = geometry;
    geometrydestroylist_fill++;
    mutex_Release(geometrydestroylist_m);
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
            rqueue, sizeof(*render_queue) * newalloc
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
        assert(rfill < ralloc);
        s3d_pos vertex_positions[3];
        spew3d_math3d_transform3d(
            geometry->vertex[
                geometry->polygon_vertexindex[ioffset]
            ],
            cam_info, effective_model_pos,
            effective_model_rot,
            &rqueue[rfill].vertex_pos[0]
        );

        // First vertex:
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
        spew3d_math3d_transform3d(
            geometry->vertex[
                geometry->polygon_vertexindex[ioffset]
            ],
            cam_info, effective_model_pos,
            effective_model_rot,
            &rqueue[rfill].vertex_pos[1]
        );
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
        spew3d_math3d_transform3d(
            geometry->vertex[
                geometry->polygon_vertexindex[ioffset]
            ],
            cam_info, effective_model_pos,
            effective_model_rot,
            &rqueue[rfill].vertex_pos[2]
        );
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

        // Misc:
        rqueue[rfill].polygon_material = (
            geometry->polygon_material[i]
        );
        memset(&rqueue[rfill].vertex_normal[0], 0,
            sizeof(rqueue[rfill].vertex_normal[0]) * 3);
        ioffset++;

        rfill++;
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

