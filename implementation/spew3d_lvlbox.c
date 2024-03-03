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

#ifdef SPEW3D_IMPLEMENTATION

#include <stdint.h>

typedef struct s3d_lvlbox_internal {
    int wasdeleted;
    s3d_mutex *m;

    char *last_used_tex;
    int last_used_tex_vfsflags;
} s3d_lvlbox_internal;

S3DHID int spew3d_lvlbox_TryUpdateTileCache_nolock(
    s3d_lvlbox *lvlbox,
    uint32_t chunk_index, uint32_t tile_index
);
S3DHID int _spew3d_lvlbox_SetFloorOrCeilOrWallTextureAt_nolock(
    s3d_lvlbox *lvlbox, s3d_pos pos,
    const char *texname, int vfsflags,
    int to_ceiling, int to_wall_no, int to_wall_top_part
);
S3DHID void _spew3d_lvlbox_TileAndChunkIndexToPos_nolock(
    s3d_lvlbox *lvlbox, uint32_t chunk_index, uint32_t tile_index,
    uint32_t *out_chunk_x, uint32_t *out_chunk_y,
    uint32_t *out_tile_x, uint32_t *out_tile_y,
    s3d_pos *out_tile_lower_end_corner
);
S3DHID int _spew3d_lvlbox_CornerPosToWorldPos_nolock(
    s3d_lvlbox *lvlbox, uint32_t _chunk_index,
    uint32_t _tile_index, int32_t _segment_no,
    int corner, int at_ceiling, s3d_pos *out_pos
);
S3DHID int _spew3d_lvlbox_GetNeighboringCorner_nolock(
    s3d_lvlbox *lvlbox, uint32_t chunk_index,
    uint32_t tile_index, int corner,
    uint32_t neighbor_chunk_index, uint32_t neighbor_tile_index,
    int *out_neighbor_corner
);
S3DHID int _spew3d_lvlbox_GetNeighborNormalsAtCorner_nolock(
    s3d_lvlbox *lvlbox, uint32_t chunk_index,
    uint32_t tile_index, int corner, int check_for_ceiling,
    uint32_t neighbor_chunk_index, uint32_t neighbor_tile_index,
    s3dnum_t floor_height, s3d_pos *out_normal
);
S3DHID int _spew3d_lvlbox_GetNeighborHeightAtCorner_nolock(
    s3d_lvlbox *lvlbox, uint32_t chunk_index,
    uint32_t tile_index, int corner, int check_for_ceiling,
    uint32_t neighbor_chunk_index, uint32_t neighbor_tile_index,
    double close_to_height, s3dnum_t *out_height
);
S3DHID int _spew3d_lvlbox_InteractPosDirToTileCorner_nolock(
    s3d_lvlbox *lvlbox, s3d_pos interact_pos,
    s3d_rotation interact_rot,
    int32_t *chunk_index, int32_t *tile_index,
    int32_t *chunk_x, int32_t *chunk_y,
    int32_t *tile_x, int32_t *tile_y,
    int32_t *segment_no, int *corner
);
S3DHID int _spew3d_lvlbox_InteractPosDirToTileCornerOrWall_nolock(
    s3d_lvlbox *lvlbox, s3d_pos interact_pos,
    s3d_rotation interact_rot,
    int32_t *out_chunk_index, int32_t *out_tile_index,
    int32_t *out_chunk_x, int32_t *out_chunk_y,
    int32_t *out_tile_x, int32_t *out_tile_y,
    int32_t *out_segment_no, int *out_corner_no,
    int *out_wall_no
);
S3DHID int _spew3d_lvlbox_ExpandToPosition_nolock(
    s3d_lvlbox *box, s3d_pos pos
);
S3DEXP int _spew3d_lvlbox_GetTileClosestCornerNearWorldPos_nolock(
    s3d_lvlbox *lvlbox, uint32_t chunk_index,
    uint32_t tile_index, int segment_no,
    s3d_pos pos, int ignore_z, int at_ceiling,
    s3d_pos *out_corner_pos
);
S3DHID int _spew3d_lvlbox_GetNeighborTile_nolock(
    s3d_lvlbox *lvlbox,
    uint32_t chunk_index, uint32_t tile_index,
    int32_t shift_x, int32_t shift_y,
    uint32_t *out_neighbor_chunk_index,
    uint32_t *out_neighbor_tile_index
);
S3DHID int32_t _spew3d_lvlbox_TileVertPosToSegmentNo_nolock(
    s3d_lvlbox *lvlbox,
    s3d_lvlbox_tile *tile, s3dnum_t pos_z,
    int ignore_lvlbox_offset
);

S3DHID static s3d_lvlbox_internal *_lvlbox_Internal(
        s3d_lvlbox *lvlbox
        ) {
    return (s3d_lvlbox_internal *)lvlbox->_internal;
}

S3DHID static void _spew3d_lvlbox_FreeTileCacheContents(
        s3d_lvlbox_tilecache *cache
        ) {
    if (!cache)
        return;
    if (cache->cached_floor != NULL)
        free(cache->cached_floor);
    cache->cached_floor_polycount = 0;
    cache->cached_floor = NULL;
    if (cache->cached_ceiling != NULL)
        free(cache->cached_ceiling);
    cache->cached_ceiling_polycount = 0;
    cache->cached_ceiling = NULL;
    if (cache->cached_wall != NULL)
        free(cache->cached_wall);
    cache->cached_wall_polycount = 0;
    cache->cached_wall = NULL;
    cache->is_up_to_date = 0;
    cache->flat_normals_set = 0;
}

S3DHID void _spew3d_lvlbox_TileAndChunkIndexToPos_nolock(
        s3d_lvlbox *lvlbox, uint32_t chunk_index,
        uint32_t tile_index,
        uint32_t *out_chunk_x, uint32_t *out_chunk_y,
        uint32_t *out_tile_x, uint32_t *out_tile_y,
        s3d_pos *out_tile_lower_end_corner
        ) {
    uint32_t chunk_x = (chunk_index %
        (uint32_t)lvlbox->chunk_extent_x);
    uint32_t chunk_y = (chunk_index - chunk_x) /
        (uint32_t)lvlbox->chunk_extent_x;
    uint32_t tile_x = (tile_index %
        (uint32_t)LVLBOX_CHUNK_SIZE);
    uint32_t tile_y = (tile_index - tile_x) /
        (uint32_t)LVLBOX_CHUNK_SIZE;
    if (out_chunk_x) *out_chunk_x = chunk_x;
    if (out_chunk_y) *out_chunk_y = chunk_y;
    if (out_tile_x) *out_tile_x = tile_x;
    if (out_tile_y) *out_tile_y = tile_y;
    if (out_tile_lower_end_corner != NULL) {
        s3d_pos pos;
        pos.x = lvlbox->offset.x + (s3dnum_t)chunk_x *
            (s3dnum_t)(LVLBOX_CHUNK_SIZE * LVLBOX_CHUNK_SIZE) +
            (s3dnum_t)tile_x * (s3dnum_t)(LVLBOX_TILE_SIZE);
        pos.y = lvlbox->offset.y + (s3dnum_t)chunk_y *
            (s3dnum_t)(LVLBOX_CHUNK_SIZE * LVLBOX_CHUNK_SIZE) +
            (s3dnum_t)tile_y * (s3dnum_t)(LVLBOX_TILE_SIZE);
        if (!lvlbox->chunk[chunk_index].tile[tile_index].occupied ||
                lvlbox->chunk[chunk_index].
                tile[tile_index].segment_count <= 0) {
            pos.z = 0;
        } else {
            s3dnum_t min_z = (
                lvlbox->chunk[chunk_index].
                    tile[tile_index].segment[0].floor_z[0]
            );
            int i = 1;
            while (i < 4) {
                s3dnum_t corner_z = (
                    lvlbox->chunk[chunk_index].
                        tile[tile_index].segment[0].floor_z[i]
                );
                min_z = fmin(min_z, corner_z);
                i++;
            }
            pos.z = min_z;
        }
        *out_tile_lower_end_corner = pos;
    }
}

S3DHID int _spew3d_lvlbox_GetNeighborTileVertSegment_nolock(
        s3d_lvlbox *lvlbox,
        uint32_t chunk_index, uint32_t tile_index,
        int segment_no,
        uint32_t shift_x, uint32_t shift_y,
        int check_step_height_at_floor,
        int check_step_height_at_ceiling,
        double max_step_height, int dont_limit_negative_steps,
        int dont_require_height_at_entire_edge,
        s3d_pos *optional_ref_pos, int ref_pos_ignores_level_offset,
        uint32_t *out_neighbor_chunk_index,
        uint32_t *out_neighbor_tile_index,
        int32_t *out_neighbor_segment_no,
        s3dnum_t *out_rel_step_height,
        s3dnum_t *out_abs_step_height
        ) {
    uint32_t neighbor_chunk_index;
    uint32_t neighbor_tile_index;
    int result = _spew3d_lvlbox_GetNeighborTile_nolock(
        lvlbox, chunk_index, tile_index, shift_x, shift_y,
        &neighbor_chunk_index, &neighbor_tile_index
    );
    if (!result)
        return 0;

    if (max_step_height < 0.01) {
        max_step_height = 0.01;
    }

    s3d_pos ref_pos = {0};
    ref_pos.x = LVLBOX_TILE_SIZE * 0.5;
    ref_pos.y = LVLBOX_TILE_SIZE * 0.5;
    if (optional_ref_pos != NULL) {
        ref_pos = *optional_ref_pos;
        if (ref_pos_ignores_level_offset) {
            ref_pos.x += lvlbox->offset.x;
            ref_pos.y += lvlbox->offset.y;
            ref_pos.z += lvlbox->offset.z;
        }
        s3d_pos tile_lower_border;
        _spew3d_lvlbox_TileAndChunkIndexToPos_nolock(
            lvlbox, chunk_index, tile_index, NULL,
            NULL, NULL, NULL, &tile_lower_border
        );
        ref_pos.x = fmax(0.0, fmin(1.0,
            ref_pos.x - tile_lower_border.x
        ));
        ref_pos.y = fmax(0.0, fmin(1.0,
            ref_pos.y - tile_lower_border.y
        ));
    }
    s3d_lvlbox_tile *our_tile = (
        &lvlbox->chunk[chunk_index].tile[tile_index]
    );
    s3d_lvlbox_tile *neighbor_tile = (
        &lvlbox->chunk[neighbor_chunk_index].tile[neighbor_tile_index]
    );
    if (!our_tile->occupied || !neighbor_tile->occupied)
        return 0;
    s3d_lvlbox_vertsegment *our_seg = &our_tile->segment[segment_no];

    double rel_step_height;
    double abs_step_height;

    int32_t neighbor_segment_no = -1;
    uint32_t i = 0;
    while (i < neighbor_tile->segment_count) {
        s3d_lvlbox_vertsegment *neighbor_seg = (
            &neighbor_tile->segment[i]
        );
        double height1, height2, height1weighting, ledge1abs, ledge2abs;
        if (shift_y < 0) {  // To left.
            if (shift_x < 0) {
                // Back left.
                if (check_step_height_at_floor) {
                    height1 = (neighbor_seg->floor_z[0] -
                        our_seg->floor_z[2]);
                    ledge1abs = neighbor_seg->floor_z[0];
                    height2 = height1;
                    ledge2abs = ledge1abs;
                    height1weighting = 1;
                }
                if (check_step_height_at_ceiling) {
                    height1 = (neighbor_seg->ceiling_z[0] -
                        our_seg->ceiling_z[2]);
                    ledge1abs = neighbor_seg->ceiling_z[0];
                    height2 = height1;
                    ledge2abs = ledge1abs;
                    height1weighting = 1;
                }
            } else if (shift_x == 0) {
                // Left side.
                if (check_step_height_at_floor) {
                    height1 = (neighbor_seg->floor_z[0] -
                        our_seg->floor_z[3]);
                    ledge1abs = neighbor_seg->floor_z[0];
                    height2 = (neighbor_seg->floor_z[1] -
                        our_seg->floor_z[2]);
                    ledge2abs = neighbor_seg->floor_z[1];
                    height1weighting = ref_pos.x;
                }
                if (check_step_height_at_ceiling) {
                    height1 = (neighbor_seg->ceiling_z[0] -
                        our_seg->ceiling_z[3]);
                    ledge1abs = neighbor_seg->ceiling_z[0];
                    height2 = (neighbor_seg->ceiling_z[1] -
                        our_seg->ceiling_z[2]);
                    ledge2abs = neighbor_seg->ceiling_z[1];
                    height1weighting = ref_pos.x;
                }
            } else {
                assert(shift_x > 0);
                // Front left.
                if (check_step_height_at_floor) {
                    height1 = (neighbor_seg->floor_z[1] -
                        our_seg->floor_z[3]);
                    ledge1abs = neighbor_seg->floor_z[1];
                    height2 = height1;
                    ledge2abs = ledge1abs;
                    height1weighting = 1;
                }
                if (check_step_height_at_ceiling) {
                    height1 = (neighbor_seg->ceiling_z[1] -
                        our_seg->ceiling_z[3]);
                    ledge1abs = neighbor_seg->ceiling_z[1];
                    height2 = height1;
                    ledge2abs = ledge1abs;
                    height1weighting = 1;
                }
            }
        } else if (shift_y == 0) {  // To front or back.
            assert(shift_x != 0);
            if (shift_x > 0) {
                // In front.
                if (check_step_height_at_floor) {
                    height1 = (neighbor_seg->floor_z[1] -
                        our_seg->floor_z[0]);
                    ledge1abs = neighbor_seg->floor_z[1];
                    height2 = (neighbor_seg->floor_z[2] -
                        our_seg->floor_z[3]);
                    ledge2abs = neighbor_seg->floor_z[2];
                    height1weighting = ref_pos.y;
                }
                if (check_step_height_at_ceiling) {
                    height1 = (neighbor_seg->ceiling_z[1] -
                        our_seg->ceiling_z[0]);
                    ledge1abs = neighbor_seg->ceiling_z[1];
                    height2 = (neighbor_seg->ceiling_z[2] -
                        our_seg->ceiling_z[3]);
                    ledge2abs = neighbor_seg->ceiling_z[2];
                    height1weighting = ref_pos.y;
                }
            } else {
                assert(shift_x < 0);
                // In the back.
                if (check_step_height_at_floor) {
                    height1 = (neighbor_seg->floor_z[0] -
                        our_seg->floor_z[1]);
                    ledge1abs = neighbor_seg->floor_z[0];
                    height2 = (neighbor_seg->floor_z[3] -
                        our_seg->floor_z[2]);
                    ledge2abs = neighbor_seg->floor_z[3];
                    height1weighting = ref_pos.y;
                }
                if (check_step_height_at_ceiling) {
                    height1 = (neighbor_seg->ceiling_z[0] -
                        our_seg->ceiling_z[1]);
                    ledge1abs = neighbor_seg->ceiling_z[0];
                    height2 = (neighbor_seg->ceiling_z[3] -
                        our_seg->ceiling_z[2]);
                    ledge2abs = neighbor_seg->ceiling_z[3];
                    height1weighting = ref_pos.y;
                }
            }
        } else {  // To the right
            assert(shift_y > 0);
            if (shift_x < 0) {
                // Back right.
                if (check_step_height_at_floor) {
                    height1 = (neighbor_seg->floor_z[3] -
                        our_seg->floor_z[1]);
                    ledge1abs = neighbor_seg->floor_z[3];
                    height2 = height1;
                    ledge2abs = ledge1abs;
                    height1weighting = 1;
                }
                if (check_step_height_at_ceiling) {
                    height1 = (neighbor_seg->ceiling_z[3] -
                        our_seg->ceiling_z[1]);
                    ledge1abs = neighbor_seg->ceiling_z[3];
                    height2 = height1;
                    ledge2abs = ledge1abs;
                    height1weighting = 1;
                }
            } else if (shift_x == 0) {
                // Right side.
                if (check_step_height_at_floor) {
                    height1 = (neighbor_seg->floor_z[2] -
                        our_seg->floor_z[1]);
                    ledge1abs = neighbor_seg->floor_z[2];
                    height2 = (neighbor_seg->floor_z[3] -
                        our_seg->floor_z[0]);
                    ledge2abs = neighbor_seg->floor_z[3];
                    height1weighting = (1 - ref_pos.x);
                }
                if (check_step_height_at_ceiling) {
                    height1 = (neighbor_seg->ceiling_z[2] -
                        our_seg->ceiling_z[1]);
                    ledge1abs = neighbor_seg->ceiling_z[2];
                    height2 = (neighbor_seg->ceiling_z[3] -
                        our_seg->ceiling_z[0]);
                    ledge2abs = neighbor_seg->ceiling_z[3];
                    height1weighting = (1 - ref_pos.x);
                }
            } else {
                assert(shift_x > 0);
                // Front right.
                if (check_step_height_at_floor) {
                    height1 = (neighbor_seg->floor_z[2] -
                        our_seg->floor_z[0]);
                    ledge1abs = neighbor_seg->floor_z[2];
                    height2 = height1;
                    ledge2abs = ledge1abs;
                    height1weighting = 1;
                }
                if (check_step_height_at_ceiling) {
                    height1 = (neighbor_seg->ceiling_z[2] -
                        our_seg->ceiling_z[0]);
                    ledge1abs = neighbor_seg->ceiling_z[2];
                    height2 = height1;
                    ledge2abs = ledge1abs;
                    height1weighting = 1;
                }
            }
        }
        int skip = 0;
        if (!dont_require_height_at_entire_edge) {
            if (height1 > max_step_height ||
                    (!dont_limit_negative_steps &&
                    fabs(height1) > max_step_height)) {
                skip = 1;
            } else {
                if (height2 > max_step_height ||
                        (!dont_limit_negative_steps &&
                        fabs(height2) > max_step_height))
                    skip = 1;
            }
            if (!skip) {
                rel_step_height = (
                    height1 * height1weighting +
                    height2 * (1.0 - height1weighting)
                );
            }
        } else {
            double relevant_height = (
                height1 * height1weighting +
                height2 * (1.0 - height1weighting)
            );
            if (relevant_height > max_step_height ||
                    (!dont_limit_negative_steps &&
                    fabs(relevant_height) > max_step_height)) {
                skip = 1;
            }
            if (!skip) {
                rel_step_height = relevant_height;
            }
        }
        if (!skip) {
            abs_step_height = (
                ledge1abs * height1weighting +
                ledge2abs * (1.0 - height1weighting)
            );
            neighbor_segment_no = i;
            break;
        }
        i++;
    }
    if (out_neighbor_chunk_index)
        *out_neighbor_chunk_index = neighbor_chunk_index;
    if (out_neighbor_tile_index)
        *out_neighbor_tile_index = neighbor_tile_index;
    if (out_neighbor_segment_no)
        *out_neighbor_segment_no = neighbor_segment_no;
    if (out_rel_step_height)
        *out_rel_step_height = rel_step_height;
    if (out_abs_step_height)
        *out_abs_step_height = abs_step_height;
    return 1;
}

S3DHID static void _spew3d_lvlbox_FreeChunkContents(
        s3d_lvlbox_chunk *chunk
        ) {
    uint32_t i = 0;
    while (i < (uint32_t)(LVLBOX_CHUNK_SIZE * LVLBOX_CHUNK_SIZE)) {
        if (!chunk->tile[i].occupied) {
            i++;
        }
        int k = 0;
        while (k < chunk->tile[i].segment_count) {
            if (chunk->tile[i].segment[k].floor_tex.name != NULL)
                free(chunk->tile[i].segment[k].floor_tex.name);
            _spew3d_lvlbox_FreeTileCacheContents(
                &chunk->tile[i].segment[k].cache
            );
            k++;
        }
        free(chunk->tile[i].segment);
        i++;
    }
}

S3DHID static void _spew3d_lvlbox_ActuallyDestroy(
        s3d_lvlbox *lvlbox
        ) {
    if (!lvlbox)
        return;

    uint32_t i = 0;
    while (i < lvlbox->chunk_count) {
        _spew3d_lvlbox_FreeChunkContents(&lvlbox->chunk[i]);
        i++;
    }
    free(lvlbox->chunk);
}

S3DHID int _spew3d_lvlbox_GetNeighborTile_nolock(
        s3d_lvlbox *lvlbox,
        uint32_t chunk_index, uint32_t tile_index,
        int32_t shift_x, int32_t shift_y,
        uint32_t *out_neighbor_chunk_index,
        uint32_t *out_neighbor_tile_index
        ) {
    uint32_t chunk_x, chunk_y, tile_x, tile_y;
    _spew3d_lvlbox_TileAndChunkIndexToPos_nolock(
        lvlbox, chunk_index, tile_index, &chunk_x,
        &chunk_y, &tile_x, &tile_y, NULL
    );

    int32_t neighbor_tile_x = (int32_t)tile_x + shift_x;
    int32_t neighbor_tile_y = (int32_t)tile_y + shift_y;
    int32_t neighbor_chunk_x = chunk_x;
    int32_t neighbor_chunk_y = chunk_y;
    while (neighbor_tile_x < 0) {
        neighbor_tile_x += LVLBOX_CHUNK_SIZE;
        neighbor_chunk_x--;
    }
    if (neighbor_chunk_x < 0)
        return 0;
    while (neighbor_tile_y < 0) {
        neighbor_tile_y += LVLBOX_CHUNK_SIZE;
        neighbor_chunk_y--;
    }
    if (neighbor_chunk_y < 0)
        return 0;
    while (neighbor_tile_x >= LVLBOX_CHUNK_SIZE) {
        neighbor_tile_x -= LVLBOX_CHUNK_SIZE;
        neighbor_chunk_x++;
    }
    if (neighbor_chunk_x > lvlbox->chunk_extent_x)
        return 0;
    while (neighbor_tile_y >= LVLBOX_CHUNK_SIZE) {
        neighbor_tile_y -= LVLBOX_CHUNK_SIZE;
        neighbor_chunk_y++;
    }
    uint32_t new_chunk_index = (
        neighbor_chunk_y * (uint32_t)lvlbox->chunk_extent_x +
        neighbor_chunk_x
    );
    if (new_chunk_index >= lvlbox->chunk_count)
        return 0;
    *out_neighbor_chunk_index = new_chunk_index;
    uint32_t new_tile_index = (
        neighbor_tile_y * (uint32_t)LVLBOX_CHUNK_SIZE +
        neighbor_tile_x
    );
    *out_neighbor_tile_index = new_tile_index;
    return 1;
}

S3DHID int _spew3d_lvlbox_GetNeighboringCorner_nolock(
        s3d_lvlbox *lvlbox, uint32_t chunk_index,
        uint32_t tile_index, int corner,
        uint32_t neighbor_chunk_index, uint32_t neighbor_tile_index,
        int *out_neighbor_corner
        ) {
    if (chunk_index >= lvlbox->chunk_count)
        return 0;
    if (tile_index >= LVLBOX_CHUNK_SIZE * LVLBOX_CHUNK_SIZE)
        return 0;
    if (neighbor_chunk_index >= lvlbox->chunk_count)
        return 0;
    if (neighbor_tile_index >= LVLBOX_CHUNK_SIZE * LVLBOX_CHUNK_SIZE)
        return 0;
    if (corner < 0 || corner >= 4)
        return 0;

    uint32_t our_chunk_x, our_chunk_y, our_tile_x, our_tile_y;
    _spew3d_lvlbox_TileAndChunkIndexToPos_nolock(
        lvlbox, chunk_index, tile_index,
        &our_chunk_x, &our_chunk_y, &our_tile_x, &our_tile_y, NULL
    );
    uint32_t neighbor_chunk_x, neighbor_chunk_y,
        neighbor_tile_x, neighbor_tile_y;
    _spew3d_lvlbox_TileAndChunkIndexToPos_nolock(
        lvlbox, neighbor_chunk_index, neighbor_tile_index,
        &neighbor_chunk_x, &neighbor_chunk_y,
        &neighbor_tile_x, &neighbor_tile_y, NULL
    );

    int32_t shift_x = (neighbor_chunk_x * LVLBOX_CHUNK_SIZE +
        neighbor_tile_x) - (our_chunk_x * LVLBOX_CHUNK_SIZE +
        our_tile_x);
    int32_t shift_y = (neighbor_chunk_y * LVLBOX_CHUNK_SIZE +
        neighbor_tile_y) - (our_chunk_y * LVLBOX_CHUNK_SIZE +
        our_tile_y);
    if (shift_x < -1 || shift_x > 1 ||
            shift_y < -1 || shift_y > 1)
        return 0;

    int neighbor_corner = corner;
    if (shift_y == -1) {
        if (shift_x == 0) {
            if (corner == 3) neighbor_corner = 0;
            else if (corner == 2) neighbor_corner = 1;
            else return 0;
        } else if (shift_x == 1) {
            if (corner == 3) neighbor_corner = 1;
            else return 0;
        } else {
            assert(shift_x == -1);
            if (corner == 2) neighbor_corner = 0;
            else return 0;
        }
    } else if (shift_y == 1) {
        if (shift_x == 0) {
            if (corner == 0) neighbor_corner = 3;
            else if (corner == 1) neighbor_corner = 2;
            else return 0;
        } else if (shift_x == 1) {
            if (corner == 0) neighbor_corner = 2;
            else return 0;
        } else {
            assert(shift_x == -1);
            if (corner == 1) neighbor_corner = 3;
            else return 0;
        }
    } else {
        assert(shift_y == 0);
        if (shift_x == 0) {
            // Nothing to change.
        } else if (shift_x == 1) {
            if (corner == 0) neighbor_corner = 1;
            else if (corner == 3) neighbor_corner = 2;
            else return 0;
        } else {
            assert(shift_x == -1);
            if (corner == 1) neighbor_corner = 0;
            else if (corner == 2) neighbor_corner = 3;
            else return 0;
        }
    }
    *out_neighbor_corner = neighbor_corner;
    return 1;
}

S3DHID int _spew3d_lvlbox_GetNeighborHeightAtCorner_nolock(
        s3d_lvlbox *lvlbox, uint32_t chunk_index,
        uint32_t tile_index, int corner, int check_for_ceiling,
        uint32_t neighbor_chunk_index, uint32_t neighbor_tile_index,
        double close_to_height, s3dnum_t *out_height
        ) {
    int neighbor_corner = -1;
    int result = _spew3d_lvlbox_GetNeighboringCorner_nolock(
        lvlbox, chunk_index, tile_index, corner,
        neighbor_chunk_index, neighbor_tile_index,
        &neighbor_corner
    );
    if (!result)
        return 0;
    assert(neighbor_corner >= 0);

    s3d_lvlbox_tile *neighbor_tile = (
        &lvlbox->chunk[neighbor_chunk_index].tile[neighbor_tile_index]
    );
    if (!neighbor_tile->occupied) {
        return 0;
    }
    assert(neighbor_tile->segment_count > 0);
    int32_t segment_no = _spew3d_lvlbox_TileVertPosToSegmentNo_nolock(
        lvlbox, neighbor_tile, close_to_height, 0);
    if (segment_no < 0)
        segment_no = 0;
    if (check_for_ceiling) {
        if (out_height) {
            *out_height = (
                neighbor_tile->segment[segment_no].
                    ceiling_z[neighbor_corner]
            );
        }
        return 1;
    } else {
        if (out_height) {
            *out_height = (
                neighbor_tile->segment[segment_no].
                    floor_z[neighbor_corner]
            );
        }
        return 1;
    }
    return 0;
}

S3DHID int _spew3d_lvlbox_GetNeighborNormalsAtCorner_nolock(
        s3d_lvlbox *lvlbox, uint32_t chunk_index,
        uint32_t tile_index, int corner, int check_for_ceiling,
        uint32_t neighbor_chunk_index, uint32_t neighbor_tile_index,
        s3dnum_t floor_height, s3d_pos *out_normal
        ) {
    int neighbor_corner = -1;
    int result = _spew3d_lvlbox_GetNeighboringCorner_nolock(
        lvlbox, chunk_index, tile_index, corner,
        neighbor_chunk_index, neighbor_tile_index,
        &neighbor_corner
    );
    if (!result)
        return 0;
    assert(neighbor_corner >= 0);

    s3d_lvlbox_tile *neighbor_tile = (
        &lvlbox->chunk[neighbor_chunk_index].tile[neighbor_tile_index]
    );
    if (!neighbor_tile->occupied)
        return 0;
    if (check_for_ceiling) {
        int32_t i = 0;
        while (i < neighbor_tile->segment_count) {
            if (fabs(neighbor_tile->segment[i].
                    ceiling_z[neighbor_corner] -
                    floor_height) <= 0.01) {
                assert(neighbor_tile->segment[i].cache.flat_normals_set);
                if (out_normal)
                    *out_normal = neighbor_tile->segment[i].cache.
                        ceiling_flat_corner_normals[neighbor_corner];
                return 1;
            }
            i++;
        }
    } else {
        int32_t i = 0;
        while (i < neighbor_tile->segment_count) {
            if (fabs(neighbor_tile->segment[i].
                    floor_z[neighbor_corner] -
                    floor_height) <= 0.01) {
                assert(neighbor_tile->segment[i].cache.flat_normals_set);
                if (out_normal)
                    *out_normal = neighbor_tile->segment[i].cache.
                        floor_flat_corner_normals[neighbor_corner];
                return 1;
            }
            i++;
        }
    }
    return 0;
}

S3DHID int _spew3d_lvlbox_TryUpdateTileCache_nolock_Ex(
        s3d_lvlbox *lvlbox,
        uint32_t chunk_index, uint32_t tile_index,
        int stop_after_flat_normals_floor
        ) {
    uint32_t chunk_x, chunk_y, tile_x, tile_y;
    s3d_pos tile_lower_end;
    _spew3d_lvlbox_TileAndChunkIndexToPos_nolock(
        lvlbox, chunk_index, tile_index, &chunk_x,
        &chunk_y, &tile_x, &tile_y, &tile_lower_end
    );

    s3d_lvlbox_tile *tile = (
        &lvlbox->chunk[chunk_index].tile[tile_index]
    );
    uint32_t i = 0;
    while (i < tile->segment_count) {
        if (tile->segment[i].cache.is_up_to_date) {
            i++;
            continue;
        }
        s3d_lvlbox_tilecache *cache = (
            &tile->segment[i].cache
        );
        const uint32_t segment_no =i;

        // Set up floor list for use:
        if (cache->cached_floor_polycount != 2) {
            cache->flat_normals_set = 0;
            s3d_lvlbox_tilepolygon *new_polys = (
                malloc(sizeof(*new_polys) * 2)
            );
            if (!new_polys) {
                return 0;
            }
            if (cache->cached_floor != NULL)
                free(cache->cached_floor);
            cache->cached_floor = new_polys;
            cache->cached_floor_polycount = 2;
        }
        if (cache->cached_ceiling_polycount != 2) {
            cache->flat_normals_set = 0;
            s3d_lvlbox_tilepolygon *new_polys = (
                malloc(sizeof(*new_polys) * 2)
            );
            if (!new_polys) {
                return 0;
            }
            if (cache->cached_ceiling != NULL)
                free(cache->cached_ceiling);
            cache->cached_ceiling = new_polys;
            cache->cached_ceiling_polycount = 2;
        }
        if (cache->cached_wall_polycount != 4 * 2) {
            cache->flat_normals_set = 0;
            s3d_lvlbox_tilepolygon *new_polys = (
                malloc(sizeof(*new_polys) * 4 * 2)
            );
            if (!new_polys) {
                return 0;
            }
            if (cache->cached_wall != NULL)
                free(cache->cached_wall);
            cache->cached_wall = new_polys;
            cache->cached_wall_polycount = 2;
        }

        // Set up flat, unsmoothed floor polygons if needed:
        if (!cache->flat_normals_set) {
            memset(&cache->cached_floor[0], 0,
                sizeof(cache->cached_floor[0]) *
                cache->cached_floor_polycount);
            cache->flat_normals_set = 1;

            // *** FLOOR: ***

            double diagonalfrontleftbackright = fabs(
                tile->segment[segment_no].floor_z[3] -
                tile->segment[segment_no].floor_z[1]
            );
            double diagonalfrontrightbackleft = fabs(
                tile->segment[segment_no].floor_z[0] -
                tile->segment[segment_no].floor_z[2]
            );

            s3d_pos back_left = tile_lower_end;
            s3d_pos back_right = back_left;
            back_right.y += (s3dnum_t)LVLBOX_TILE_SIZE;
            s3d_pos front_left = back_left;
            front_left.x += (s3dnum_t)LVLBOX_TILE_SIZE;
            s3d_pos front_right = back_right;
            front_right.x += (s3dnum_t)LVLBOX_TILE_SIZE;
            front_right.z = tile->segment[segment_no].floor_z[0];
            back_right.z = tile->segment[segment_no].floor_z[1];
            back_left.z = tile->segment[segment_no].floor_z[2];
            front_left.z = tile->segment[segment_no].floor_z[3];

            s3d_point texcoord_back_left = {0};
            texcoord_back_left.y = 1.0;
            s3d_point texcoord_back_right = {0};
            texcoord_back_right.x = 1.0;
            texcoord_back_right.y = 1.0;
            s3d_point texcoord_front_left = {0};
            s3d_point texcoord_front_right = {0};
            texcoord_front_right.x = 1.0;

            if (diagonalfrontrightbackleft >
                    diagonalfrontleftbackright &&
                    tile->segment[segment_no].floor_tex.id != 0) {
                cache->floor_split_from_front_left = 1;
                cache->cached_floor[0].texture =
                    tile->segment[segment_no].floor_tex.id;
                cache->cached_floor[0].material =
                    tile->segment[segment_no].floor_tex.material;
                cache->cached_floor[0].vertex[0] = front_left;
                cache->cached_floor[0].vertex[1] = front_right;
                cache->cached_floor[0].vertex[2] = back_right;
                cache->cached_floor[0].texcoord[0] =
                    texcoord_front_left;
                cache->cached_floor[0].texcoord[1] =
                    texcoord_front_right;
                cache->cached_floor[0].texcoord[2] =
                    texcoord_back_right;
                spew3d_math3d_polygon_normal(
                    &cache->cached_floor[0].vertex[0],
                    &cache->cached_floor[0].vertex[1],
                    &cache->cached_floor[0].vertex[2],
                    1, &cache->cached_floor[0].polynormal
                );
                if (cache->cached_floor[0].polynormal.z < 0)
                    spew3d_math3d_flip(
                        &cache->cached_floor[0].polynormal
                    );

                cache->cached_floor[1].texture =
                    tile->segment[segment_no].floor_tex.id;
                cache->cached_floor[1].material =
                    tile->segment[segment_no].floor_tex.material;
                cache->cached_floor[1].vertex[0] = back_right;
                cache->cached_floor[1].vertex[1] = back_left;
                cache->cached_floor[1].vertex[2] = front_left;
                cache->cached_floor[1].texcoord[0] =
                    texcoord_back_right;
                cache->cached_floor[1].texcoord[1] =
                    texcoord_back_left;
                cache->cached_floor[1].texcoord[2] =
                    texcoord_front_left;
                spew3d_math3d_polygon_normal(
                    &cache->cached_floor[1].vertex[0],
                    &cache->cached_floor[1].vertex[1],
                    &cache->cached_floor[1].vertex[2],
                    1, &cache->cached_floor[1].polynormal
                );
                if (cache->cached_floor[1].polynormal.z < 0)
                    spew3d_math3d_flip(
                        &cache->cached_floor[1].polynormal
                    );

                cache->floor_flat_corner_normals[0] =
                    cache->cached_floor[0].polynormal;
                cache->floor_flat_corner_normals[2] =
                    cache->cached_floor[1].polynormal;
                cache->floor_flat_corner_normals[1] = spew3d_math3d_average(
                    &cache->cached_floor[0].polynormal,
                    &cache->cached_floor[1].polynormal
                );
                cache->floor_flat_corner_normals[3] = spew3d_math3d_average(
                    &cache->cached_floor[0].polynormal,
                    &cache->cached_floor[1].polynormal
                );
            } else if (tile->segment[segment_no].floor_tex.id != 0) {
                cache->floor_split_from_front_left = 0;
                cache->cached_floor[0].texture =
                    tile->segment[segment_no].floor_tex.id;
                cache->cached_floor[0].material =
                    tile->segment[segment_no].floor_tex.material;
                cache->cached_floor[0].vertex[0] = front_left;
                cache->cached_floor[0].vertex[1] = front_right;
                cache->cached_floor[0].vertex[2] = back_left;
                cache->cached_floor[0].texcoord[0] =
                    texcoord_front_left;
                cache->cached_floor[0].texcoord[1] =
                    texcoord_front_right;
                cache->cached_floor[0].texcoord[2] =
                    texcoord_back_left;
                spew3d_math3d_polygon_normal(
                    &cache->cached_floor[0].vertex[0],
                    &cache->cached_floor[0].vertex[1],
                    &cache->cached_floor[0].vertex[2],
                    1, &cache->cached_floor[0].polynormal
                );
                if (cache->cached_floor[0].polynormal.z < 0)
                    spew3d_math3d_flip(
                        &cache->cached_floor[0].polynormal
                    );

                cache->cached_floor[1].texture =
                    tile->segment[segment_no].floor_tex.id;
                cache->cached_floor[1].material =
                    tile->segment[segment_no].floor_tex.material;
                cache->cached_floor[1].vertex[0] = back_left;
                cache->cached_floor[1].vertex[1] = front_right;
                cache->cached_floor[1].vertex[2] = back_right;
                cache->cached_floor[1].texcoord[0] =
                    texcoord_back_left;
                cache->cached_floor[1].texcoord[1] =
                    texcoord_front_right;
                cache->cached_floor[1].texcoord[2] =
                    texcoord_back_right;
                spew3d_math3d_polygon_normal(
                    &cache->cached_floor[1].vertex[0],
                    &cache->cached_floor[1].vertex[1],
                    &cache->cached_floor[1].vertex[2],
                    1, &cache->cached_floor[1].polynormal
                );
                if (cache->cached_floor[1].polynormal.z < 0)
                    spew3d_math3d_flip(
                        &cache->cached_floor[1].polynormal
                    );

                cache->floor_flat_corner_normals[1] =
                    cache->cached_floor[1].polynormal;
                cache->floor_flat_corner_normals[3] =
                    cache->cached_floor[0].polynormal;
                cache->floor_flat_corner_normals[0] = spew3d_math3d_average(
                    &cache->cached_floor[0].polynormal,
                    &cache->cached_floor[1].polynormal
                );
                cache->floor_flat_corner_normals[2] = spew3d_math3d_average(
                    &cache->cached_floor[0].polynormal,
                    &cache->cached_floor[1].polynormal
                );
            }

            // ** CEILING: **

            diagonalfrontleftbackright = fabs(
                tile->segment[segment_no].ceiling_z[3] -
                tile->segment[segment_no].ceiling_z[1]
            );
            diagonalfrontrightbackleft = fabs(
                tile->segment[segment_no].ceiling_z[0] -
                tile->segment[segment_no].ceiling_z[2]
            );

            back_left = tile_lower_end;
            back_right = back_left;
            back_right.y += (s3dnum_t)LVLBOX_TILE_SIZE;
            front_left = back_left;
            front_left.x += (s3dnum_t)LVLBOX_TILE_SIZE;
            front_right = back_right;
            front_right.x += (s3dnum_t)LVLBOX_TILE_SIZE;
            front_right.z = tile->segment[segment_no].ceiling_z[0];
            back_right.z = tile->segment[segment_no].ceiling_z[1];
            back_left.z = tile->segment[segment_no].ceiling_z[2];
            front_left.z = tile->segment[segment_no].ceiling_z[3];

            memset(&texcoord_back_left, 0, sizeof(texcoord_back_left));
            texcoord_back_left.y = 1.0;
            memset(&texcoord_back_right, 0, sizeof(texcoord_back_right));
            texcoord_back_right.x = 1.0;
            texcoord_back_right.y = 1.0;
            memset(&texcoord_front_left, 0, sizeof(texcoord_front_left));
            memset(&texcoord_front_right, 0, sizeof(texcoord_front_right));
            texcoord_front_right.x = 1.0;

            if (diagonalfrontrightbackleft >
                    diagonalfrontleftbackright &&
                    tile->segment[segment_no].ceiling_tex.id != 0) {
                cache->floor_split_from_front_left = 1;
                cache->cached_ceiling[0].texture =
                    tile->segment[segment_no].ceiling_tex.id;
                cache->cached_ceiling[0].material =
                    tile->segment[segment_no].ceiling_tex.material;
                cache->cached_ceiling[0].vertex[0] = front_left;
                cache->cached_ceiling[0].vertex[1] = front_right;
                cache->cached_ceiling[0].vertex[2] = back_right;
                cache->cached_ceiling[0].texcoord[0] =
                    texcoord_front_left;
                cache->cached_ceiling[0].texcoord[1] =
                    texcoord_front_right;
                cache->cached_ceiling[0].texcoord[2] =
                    texcoord_back_right;
                spew3d_math3d_polygon_normal(
                    &cache->cached_ceiling[0].vertex[0],
                    &cache->cached_ceiling[0].vertex[1],
                    &cache->cached_ceiling[0].vertex[2],
                    1, &cache->cached_ceiling[0].polynormal
                );
                if (cache->cached_ceiling[0].polynormal.z > 0)
                    spew3d_math3d_flip(
                        &cache->cached_ceiling[0].polynormal
                    );

                cache->cached_ceiling[1].texture =
                    tile->segment[segment_no].ceiling_tex.id;
                cache->cached_ceiling[1].material =
                    tile->segment[segment_no].ceiling_tex.material;
                cache->cached_ceiling[1].vertex[0] = back_right;
                cache->cached_ceiling[1].vertex[1] = back_left;
                cache->cached_ceiling[1].vertex[2] = front_left;
                cache->cached_ceiling[1].texcoord[0] =
                    texcoord_back_right;
                cache->cached_ceiling[1].texcoord[1] =
                    texcoord_back_left;
                cache->cached_ceiling[1].texcoord[2] =
                    texcoord_front_left;
                spew3d_math3d_polygon_normal(
                    &cache->cached_ceiling[1].vertex[0],
                    &cache->cached_ceiling[1].vertex[1],
                    &cache->cached_ceiling[1].vertex[2],
                    1, &cache->cached_ceiling[1].polynormal
                );
                if (cache->cached_ceiling[1].polynormal.z > 0)
                    spew3d_math3d_flip(
                        &cache->cached_ceiling[1].polynormal
                    );

                cache->ceiling_flat_corner_normals[0] =
                    cache->cached_ceiling[0].polynormal;
                cache->ceiling_flat_corner_normals[2] =
                    cache->cached_ceiling[1].polynormal;
                cache->ceiling_flat_corner_normals[1] = (
                    spew3d_math3d_average(
                        &cache->cached_ceiling[0].polynormal,
                        &cache->cached_ceiling[1].polynormal
                    )
                );
                cache->ceiling_flat_corner_normals[3] = (
                    spew3d_math3d_average(
                        &cache->cached_ceiling[0].polynormal,
                        &cache->cached_ceiling[1].polynormal
                    )
                );
            } else if (tile->segment[segment_no].ceiling_tex.id != 0) {
                cache->floor_split_from_front_left = 0;
                cache->cached_ceiling[0].texture =
                    tile->segment[segment_no].ceiling_tex.id;
                cache->cached_ceiling[0].material =
                    tile->segment[segment_no].ceiling_tex.material;
                cache->cached_ceiling[0].vertex[0] = front_left;
                cache->cached_ceiling[0].vertex[1] = front_right;
                cache->cached_ceiling[0].vertex[2] = back_left;
                cache->cached_ceiling[0].texcoord[0] =
                    texcoord_front_left;
                cache->cached_ceiling[0].texcoord[1] =
                    texcoord_front_right;
                cache->cached_ceiling[0].texcoord[2] =
                    texcoord_back_left;
                spew3d_math3d_polygon_normal(
                    &cache->cached_ceiling[0].vertex[0],
                    &cache->cached_ceiling[0].vertex[1],
                    &cache->cached_ceiling[0].vertex[2],
                    1, &cache->cached_ceiling[0].polynormal
                );
                if (cache->cached_ceiling[0].polynormal.z < 0)
                    spew3d_math3d_flip(
                        &cache->cached_ceiling[0].polynormal
                    );

                cache->cached_ceiling[1].texture =
                    tile->segment[segment_no].ceiling_tex.id;
                cache->cached_ceiling[1].material =
                    tile->segment[segment_no].ceiling_tex.material;
                cache->cached_ceiling[1].vertex[0] = back_left;
                cache->cached_ceiling[1].vertex[1] = front_right;
                cache->cached_ceiling[1].vertex[2] = back_right;
                cache->cached_ceiling[1].texcoord[0] =
                    texcoord_back_left;
                cache->cached_ceiling[1].texcoord[1] =
                    texcoord_front_right;
                cache->cached_ceiling[1].texcoord[2] =
                    texcoord_back_right;
                spew3d_math3d_polygon_normal(
                    &cache->cached_ceiling[1].vertex[0],
                    &cache->cached_ceiling[1].vertex[1],
                    &cache->cached_ceiling[1].vertex[2],
                    1, &cache->cached_ceiling[1].polynormal
                );
                if (cache->cached_ceiling[1].polynormal.z < 0)
                    spew3d_math3d_flip(
                        &cache->cached_ceiling[1].polynormal
                    );

                cache->ceiling_flat_corner_normals[1] =
                    cache->cached_ceiling[1].polynormal;
                cache->ceiling_flat_corner_normals[3] =
                    cache->cached_ceiling[0].polynormal;
                cache->ceiling_flat_corner_normals[0] = (
                    spew3d_math3d_average(
                        &cache->cached_ceiling[0].polynormal,
                        &cache->cached_ceiling[1].polynormal
                    )
                );
                cache->ceiling_flat_corner_normals[2] = (
                    spew3d_math3d_average(
                        &cache->cached_ceiling[0].polynormal,
                        &cache->cached_ceiling[1].polynormal
                    )
                );
            }
        }
        i++;
    }
    if (stop_after_flat_normals_floor)
        return 1;
    i = 0;
    while (i < tile->segment_count) {
        if (tile->segment[i].cache.is_up_to_date) {
            i++;
            continue;
        }
        s3d_lvlbox_tilecache *cache = (
            &tile->segment[i].cache
        );
        const uint32_t segment_no = i;

        // Set up smooth, complex floor polygons:
        assert(cache->flat_normals_set);
        assert(cache->cached_floor_polycount >= 2);
        s3d_pos final_neighbor_normals[4] = {0};
        int corner = 0;
        while (corner < 4) {
            final_neighbor_normals[corner] =
                tile->segment[i].cache.floor_flat_corner_normals[0];
            int corners_collected = 1;

            int32_t x = -2;
            while (x < 1) {  // First, ensure neighbors have normals:
                x++;
                int32_t y = -2;
                while (y < 1) {
                    y++;
                    if (x == 0 && y == 0)
                        continue;
                    int32_t neighbor_tile_x = (int32_t)tile_x + x;
                    int32_t neighbor_tile_y = (int32_t)tile_y + y;
                    int32_t neighbor_chunk_x = chunk_x;
                    int32_t neighbor_chunk_y = chunk_y;
                    while (neighbor_tile_x < 0) {
                        neighbor_tile_x += LVLBOX_CHUNK_SIZE;
                        neighbor_chunk_x--;
                    }
                    while (neighbor_tile_x >= LVLBOX_CHUNK_SIZE) {
                        neighbor_tile_x -= LVLBOX_CHUNK_SIZE;
                        neighbor_chunk_x++;
                    }
                    while (neighbor_tile_y < 0) {
                        neighbor_tile_y += LVLBOX_CHUNK_SIZE;
                        neighbor_chunk_y--;
                    }
                    while (neighbor_tile_y >= LVLBOX_CHUNK_SIZE) {
                        neighbor_tile_y -= LVLBOX_CHUNK_SIZE;
                        neighbor_chunk_y++;
                    }
                    if (neighbor_chunk_x < 0 || neighbor_chunk_y < 0)
                        continue;
                    uint32_t neighbor_chunk_index = neighbor_chunk_y *
                        lvlbox->chunk_extent_x + neighbor_chunk_x;
                    uint32_t neighbor_tile_index = neighbor_tile_y *
                        LVLBOX_CHUNK_SIZE + neighbor_tile_x;
                    if (neighbor_chunk_index >= lvlbox->chunk_count ||
                            neighbor_tile_index >=
                            LVLBOX_CHUNK_SIZE * LVLBOX_CHUNK_SIZE)
                        continue;
                    double height = tile->segment[i].floor_z[corner];
                    int have_neighbor = (
                        _spew3d_lvlbox_TryUpdateTileCache_nolock_Ex(
                            lvlbox, neighbor_chunk_index,
                            neighbor_tile_index,
                            1  // Important to avoid infinite recursion.
                        )
                    );
                    if (!have_neighbor)
                        return 0;

                    s3d_pos corner_normal;
                    int r = _spew3d_lvlbox_GetNeighborNormalsAtCorner_nolock(
                        lvlbox, chunk_index, tile_index, corner, 0,
                        neighbor_chunk_index, neighbor_tile_index,
                        height, &corner_normal
                    );
                    if (!r)
                        continue;
                    final_neighbor_normals[corner].x += corner_normal.x;
                    final_neighbor_normals[corner].y += corner_normal.y;
                    final_neighbor_normals[corner].z += corner_normal.z;
                    corners_collected++;
                    y++;
                }
                x++;
            }
            if (corners_collected > 1) {
                final_neighbor_normals[corner].x /= (
                    (s3dnum_t)corners_collected
                );
                final_neighbor_normals[corner].y /= (
                    (s3dnum_t)corners_collected
                );
                final_neighbor_normals[corner].z /= (
                    (s3dnum_t)corners_collected
                );
            }
            spew3d_math3d_normalize(
                &final_neighbor_normals[corner]
            );
            corner++;
        }
        memcpy(
            &tile->segment[i].cache.floor_flat_corner_normals,
            &final_neighbor_normals,
            sizeof(s3d_pos) * 4
        );
        tile->segment[i].cache.is_up_to_date = 1;
        i++;
    }
}

S3DHID int spew3d_lvlbox_TryUpdateTileCache_nolock(
        s3d_lvlbox *lvlbox, uint32_t chunk_index, uint32_t tile_index
        ) {
    return _spew3d_lvlbox_TryUpdateTileCache_nolock_Ex(
        lvlbox, chunk_index, tile_index, 0
    );
}

S3DEXP void spew3d_lvlbox_Destroy(
        s3d_lvlbox *lvlbox
        ) {
    if (!lvlbox)
        return;

    assert(!_lvlbox_Internal(lvlbox)->wasdeleted);
    _lvlbox_Internal(lvlbox)->wasdeleted = 1;
    int result = spew3d_Deletion_Queue(DELETION_LVLBOX, lvlbox);
}

S3DHID int32_t _spew3d_lvlbox_WorldPosToChunkIndex_nolock(
        s3d_lvlbox *lvlbox, s3d_pos pos, int ignore_lvlbox_offset
        ) {
    if (lvlbox->chunk_count < 0)
        return -1;

    if (!ignore_lvlbox_offset) {
        pos.x -= lvlbox->offset.x;
        pos.y -= lvlbox->offset.y;
        pos.z -= lvlbox->offset.z;
    }

    int32_t chunk_x = floor((s3dnum_t)pos.x /
        (((s3dnum_t)LVLBOX_TILE_SIZE) *
        ((s3dnum_t)LVLBOX_CHUNK_SIZE)));
    int32_t chunk_y = floor((s3dnum_t)pos.y /
        (((s3dnum_t)LVLBOX_TILE_SIZE) *
        ((s3dnum_t)LVLBOX_CHUNK_SIZE)));
    if (chunk_x < 0 || chunk_x >= lvlbox->chunk_extent_x)
        return -1;
    if (chunk_y < 0)
        return -1;
    uint32_t idx = chunk_y * lvlbox->chunk_extent_x + chunk_x;
    if (idx >= lvlbox->chunk_count)
        return -1;
    return idx;
}

S3DHID double _spew3d_lvlbox_TileFloorHeightAtInSegment_nolock(
        s3d_lvlbox *lvlbox, s3d_pos local_pos,
        int32_t chunk_index,
        int32_t tile_index, int segment_no
        ) {
    assert(lvlbox != NULL);
    s3d_lvlbox_tile *tile = (
        &lvlbox->chunk[chunk_index].tile[tile_index]
    );
    assert(segment_no >= 0 && segment_no < tile->segment_count);
    local_pos.x = fmax(0, fmin((double)LVLBOX_TILE_SIZE, local_pos.x));
    local_pos.y = fmax(0, fmin((double)LVLBOX_TILE_SIZE, local_pos.y));
    double gradient_x = local_pos.x / (double)LVLBOX_TILE_SIZE;
    double gradient_y = local_pos.y / (double)LVLBOX_TILE_SIZE;
    double result_xpos = (
        (1 - gradient_y) *
        tile->segment[segment_no].floor_z[3] +  // front-left
        (gradient_y) *
        tile->segment[segment_no].floor_z[0]  // front-right
    );
    double result_xneg = (
        (1 - gradient_y) *
        tile->segment[segment_no].floor_z[2] +  // back-left
        (gradient_y) *
        tile->segment[segment_no].floor_z[1]  // back-right
    );
    return gradient_x * result_xpos +
        (1 - gradient_x) * result_xneg;
}

S3DEXP double spew3d_lvlbox_TileFloorHeightAtInSegment(
        s3d_lvlbox *lvlbox, s3d_pos local_pos,
        int32_t chunk_index,
        int32_t tile_index, int segment_no
        ) {
    mutex_Lock(_lvlbox_Internal(lvlbox)->m);
    double result = _spew3d_lvlbox_TileFloorHeightAtInSegment_nolock(
        lvlbox, local_pos, chunk_index, tile_index, segment_no
    );
    mutex_Release(_lvlbox_Internal(lvlbox)->m);
    return result;
}

S3DHID int32_t _spew3d_lvlbox_TileVertPosToSegmentNo_nolock(
        s3d_lvlbox *lvlbox,
        s3d_lvlbox_tile *tile, s3dnum_t pos_z,
        int ignore_lvlbox_offset
        ) {
    if (!tile->occupied)
        return -1;
    if (!ignore_lvlbox_offset)
        pos_z -= lvlbox->offset.z;
    int i = 0;
    while (i < tile->segment_count) {
        double floor_min_z = INT32_MAX;
        double ceiling_max_z = INT32_MIN;
        int match = 0;
        int k = 0;
        while (k < 4) {
            floor_min_z = fmin(floor_min_z,
                tile->segment[i].floor_z[k]);
            ceiling_max_z = fmax(ceiling_max_z,
                tile->segment[i].ceiling_z[k]);
            k++;
        }
        if ((pos_z > floor_min_z || i == 0) &&
                (pos_z < floor_min_z ||
                i >= tile->segment_count - 1)) {
            break;
        }
        i++;
    }
    assert(i < tile->segment_count);
    return i;
}

S3DHID int _spew3d_lvlbox_WorldPosToTilePos_nolock(
        s3d_lvlbox *lvlbox, s3d_pos pos, int ignore_lvlbox_offset,
        uint32_t *out_chunk_index, uint32_t *out_tile_index,
        s3d_pos *out_tile_lower_bound,
        s3d_pos *out_tile_pos_offset,
        int32_t *out_segment_no
        ) {
    if (lvlbox->chunk_count < 0)
        return 0;

    if (!ignore_lvlbox_offset) {
        pos.x -= lvlbox->offset.x;
        pos.y -= lvlbox->offset.y;
        pos.z -= lvlbox->offset.z;
    }
    int32_t chunk_index = _spew3d_lvlbox_WorldPosToChunkIndex_nolock(
        lvlbox, pos, 1
    );
    if (chunk_index < 0)
        return 0;
    assert(chunk_index < lvlbox->chunk_count);

    int32_t chunk_x = (chunk_index % lvlbox->chunk_extent_x);
    int32_t chunk_y = (chunk_index - chunk_x) / lvlbox->chunk_extent_x;
    s3dnum_t chunk_offset_x = (s3dnum_t)chunk_x * (
        ((s3dnum_t)LVLBOX_TILE_SIZE) *
        ((s3dnum_t)LVLBOX_CHUNK_SIZE));
    s3dnum_t chunk_offset_y = (s3dnum_t)chunk_y * (
        ((s3dnum_t)LVLBOX_TILE_SIZE) *
        ((s3dnum_t)LVLBOX_CHUNK_SIZE));
    pos.x -= chunk_offset_x;
    pos.y -= chunk_offset_y;
    int32_t tile_x = floor(
        (s3dnum_t)(pos.x) / ((s3dnum_t)LVLBOX_TILE_SIZE)
    );
    int32_t tile_y = floor(
        (s3dnum_t)(pos.y) / ((s3dnum_t)LVLBOX_TILE_SIZE)
    );
    if (tile_x < 0) tile_x = 0;
    if (tile_y < 0) tile_y = 0;
    if (tile_x >= (int32_t)LVLBOX_CHUNK_SIZE)
        tile_x = (int32_t)LVLBOX_CHUNK_SIZE - 1;
    if (tile_y >= (int32_t)LVLBOX_CHUNK_SIZE)
        tile_y = (int32_t)LVLBOX_CHUNK_SIZE - 1;
    int32_t tile_index = (
        tile_y * (int32_t)LVLBOX_CHUNK_SIZE + tile_x
    );
    assert(tile_index >= 0 &&
        tile_index < (int32_t)(LVLBOX_CHUNK_SIZE * LVLBOX_CHUNK_SIZE));
    if (out_chunk_index != NULL)
        *out_chunk_index = chunk_index;
    if (out_tile_index != NULL)
        *out_tile_index = tile_index;
    if (out_tile_pos_offset != NULL || out_segment_no != NULL) {
        s3d_lvlbox_tile *tile = (
            &lvlbox->chunk[chunk_index].tile[tile_index]
        );
        s3d_pos offset = {0};
        offset.x = pos.x - (s3dnum_t)tile_x *
            (s3dnum_t)LVLBOX_TILE_SIZE;
        offset.y = pos.y - (s3dnum_t)tile_y *
            (s3dnum_t)LVLBOX_TILE_SIZE;
        int32_t segment_no = (
            _spew3d_lvlbox_TileVertPosToSegmentNo_nolock(
                lvlbox, tile, offset.z, 1
            )
        );
        if (out_tile_pos_offset != NULL) {
            if (segment_no >= 0) {
                offset.z -= (
                    _spew3d_lvlbox_TileFloorHeightAtInSegment_nolock(
                        lvlbox, offset, chunk_index, tile_index, segment_no
                    ));
            }
            *out_tile_pos_offset = offset;
        }
        if (out_segment_no != NULL)
            *out_segment_no = segment_no;
    }
    if (out_tile_lower_bound != NULL) {
        s3d_pos tile_lower_bound = {0};
        tile_lower_bound.x = ((s3dnum_t)tile_x *
            (s3dnum_t)LVLBOX_TILE_SIZE) +
            (s3dnum_t)chunk_x * (
                (s3dnum_t)LVLBOX_TILE_SIZE *
                (s3dnum_t)LVLBOX_CHUNK_SIZE) +
            lvlbox->offset.x;
        tile_lower_bound.y = ((s3dnum_t)tile_y *
            (s3dnum_t)LVLBOX_TILE_SIZE) +
            (s3dnum_t)chunk_y * (
                (s3dnum_t)LVLBOX_TILE_SIZE *
                (s3dnum_t)LVLBOX_CHUNK_SIZE) +
            lvlbox->offset.y;
        tile_lower_bound.z = lvlbox->offset.z;
        if (lvlbox->chunk[chunk_index].tile[tile_index].occupied) {
            double min_z = INT32_MAX;
            int k = 0;
            while (k < 4) {
                min_z = fmin(min_z,
                    lvlbox->chunk[chunk_index].
                    tile[tile_index].segment[0].floor_z[k]);
                k++;
            }
            tile_lower_bound.z = min_z;
        }
        *out_tile_lower_bound = tile_lower_bound;
    }
    return 1;
}

S3DEXP int spew3d_lvlbox_WorldPosToTilePos(
        s3d_lvlbox *lvlbox, s3d_pos pos, int ignore_lvlbox_offset,
        uint32_t *out_chunk_index, uint32_t *out_tile_index,
        s3d_pos *out_tile_lower_bound,
        s3d_pos *out_tile_pos_offset,
        int32_t *out_segment_no
        ) {
    mutex_Lock(_lvlbox_Internal(lvlbox)->m);
    int32_t idx = _spew3d_lvlbox_WorldPosToTilePos_nolock(
        lvlbox, pos, ignore_lvlbox_offset,
        out_chunk_index, out_tile_index,
        out_tile_lower_bound, out_tile_pos_offset,
        out_segment_no
    );
    mutex_Release(_lvlbox_Internal(lvlbox)->m);
    return idx;
}

S3DHID int _spew3d_lvlbox_ShiftChunks_nolock(
        s3d_lvlbox *lvlbox,
        int32_t shift_x, int32_t shift_y
        ) {
    int32_t chunk_size_x = lvlbox->chunk_extent_x;
    int32_t chunk_size_y = lvlbox->chunk_count /
        lvlbox->chunk_extent_x;
    s3d_lvlbox_chunk *new_chunk = malloc(
        sizeof(*lvlbox->chunk) * chunk_size_x * chunk_size_y);
    if (!new_chunk)
        return 0;
    memset(new_chunk, 0, sizeof(*lvlbox->chunk) *
        chunk_size_x * chunk_size_y);

    // Copy over things to new arena in shifted position:
    int32_t x = 0;
    while (x < chunk_size_x) {
        int32_t y = 0;
        while (y < chunk_size_y) {
            uint32_t chunk_offset_old = (
                (chunk_size_x * y) + x
            );
            if (x + shift_x < 0 || x + shift_x >= chunk_size_x ||
                    y + shift_y < 0 || y + shift_y >= chunk_size_y) {
                _spew3d_lvlbox_FreeChunkContents(
                    &lvlbox->chunk[chunk_offset_old]
                );
                y++;
                continue;
            }
            uint32_t chunk_offset_new = (
                (chunk_size_x * (y + shift_y)) +
                (x + shift_x)
            );
            _spew3d_lvlbox_FreeChunkContents(
                &lvlbox->chunk[chunk_offset_new]
            );
            memcpy(&new_chunk[chunk_offset_new],
                &lvlbox->chunk[chunk_offset_old],
                sizeof(*lvlbox->chunk));
            memset(&lvlbox->chunk[chunk_offset_old], 0,
                sizeof(*lvlbox->chunk));
            y++;
        }
        x++;
    }
    // Free remaining old stuff in old lvlbox->chunk arena:
    x = 0;
    while (x < chunk_size_x) {
        int32_t y = 0;
        while (y < chunk_size_y) {
            uint32_t chunk_offset_old = (
                (chunk_size_x * y) + x
            );
            _spew3d_lvlbox_FreeChunkContents(
                &lvlbox->chunk[chunk_offset_old]
            );
            memset(&lvlbox->chunk[chunk_offset_old], 0,
                sizeof(*lvlbox->chunk));
            y++;
        }
        x++;
    }
    free(lvlbox->chunk);
    lvlbox->chunk = new_chunk;
    return 1;
}

S3DHID int _spew3d_lvlbox_ResizeChunksY_nolock(
        s3d_lvlbox *lvlbox, uint32_t chunk_y
        ) {
    uint32_t old_chunk_extent_x = lvlbox->chunk_extent_x;
    uint32_t old_chunk_extent_y = (
        lvlbox->chunk_count / lvlbox->chunk_extent_x
    );
    assert(old_chunk_extent_y >= 1);
    if (old_chunk_extent_y == chunk_y)
        return 1;
    assert(chunk_y >= 1);
    s3d_lvlbox_chunk *new_chunk = malloc(
        sizeof(*lvlbox->chunk) * old_chunk_extent_x * chunk_y);
    if (!new_chunk)
        return 0;
    memset(new_chunk, 0, sizeof(*lvlbox->chunk) *
        old_chunk_extent_x * chunk_y);
    uint32_t x = 0;
    while (x < old_chunk_extent_x) {
        uint32_t y = 0;
        while (y < old_chunk_extent_y && y < chunk_y) {
            uint32_t chunk_offset_new = (
                (old_chunk_extent_x * y) + x
            );
            uint32_t chunk_offset_old = (
                (old_chunk_extent_x * y) + x
            );
            memcpy(&new_chunk[chunk_offset_new],
                &lvlbox->chunk[chunk_offset_old],
                sizeof(*lvlbox->chunk));
            memset(&lvlbox->chunk[chunk_offset_old], 0,
                sizeof(*lvlbox->chunk));
            y++;
        }
        x++;
    }
    x = 0;
    while (x < old_chunk_extent_x) {
        // If we shrink, then we have to free the old chunks:
        uint32_t y = chunk_y;
        while (y < old_chunk_extent_y) {
            uint32_t chunk_offset_old = (
                (old_chunk_extent_x * y) + x
            );
            _spew3d_lvlbox_FreeChunkContents(
                &lvlbox->chunk[chunk_offset_old]
            );
            y++;
        }
        x++;
    }
    free(lvlbox->chunk);
    lvlbox->chunk = new_chunk;
    lvlbox->chunk_count = old_chunk_extent_x * chunk_y;
    return 1;
}

S3DHID int _spew3d_lvlbox_ResizeChunksX_nolock(
        s3d_lvlbox *lvlbox, uint32_t chunk_x
        ) {
    uint32_t old_chunk_extent_x = lvlbox->chunk_extent_x;
    if (old_chunk_extent_x == chunk_x)
        return 1;
    assert(chunk_x >= 1);
    uint32_t old_chunk_extent_y = (lvlbox->chunk_count /
        old_chunk_extent_x);
    assert(old_chunk_extent_y >= 1);
    s3d_lvlbox_chunk *new_chunk = malloc(
        sizeof(*lvlbox->chunk) * chunk_x * old_chunk_extent_y);
    if (!new_chunk)
        return 0;
    memset(new_chunk, 0, sizeof(*lvlbox->chunk) *
        chunk_x * old_chunk_extent_y);
    uint32_t x = 0;
    while (x < old_chunk_extent_x && x < chunk_x) {
        uint32_t y = 0;
        while (y < old_chunk_extent_y) {
            uint32_t chunk_offset_new = (
                (chunk_x * y) + x
            );
            uint32_t chunk_offset_old = (
                (old_chunk_extent_x * y) + x
            );
            memcpy(&new_chunk[chunk_offset_new],
                &lvlbox->chunk[chunk_offset_old],
                sizeof(*lvlbox->chunk));
            memset(&lvlbox->chunk[chunk_offset_old], 0,
                sizeof(*lvlbox->chunk));
            y++;
        }
        x++;
    }
    x = chunk_x;
    while (x < old_chunk_extent_x) {
        // If we shrink, then we have to free the old chunks:
        uint32_t y = 0;
        while (y < old_chunk_extent_y) {
            uint32_t chunk_offset_old = (
                (old_chunk_extent_x * y) + x
            );
            _spew3d_lvlbox_FreeChunkContents(
                &lvlbox->chunk[chunk_offset_old]
            );
            y++;
        }
        x++;
    }
    free(lvlbox->chunk);
    lvlbox->chunk = new_chunk;
    lvlbox->chunk_extent_x = chunk_x;
    lvlbox->chunk_count = chunk_x * old_chunk_extent_y;
    return 1;
}

S3DEXP int32_t spew3d_lvlbox_WorldPosToChunkIndex(
        s3d_lvlbox *lvlbox, s3d_pos pos, int ignore_lvlbox_offset
        ) {
    mutex_Lock(_lvlbox_Internal(lvlbox)->m);
    int32_t idx = _spew3d_lvlbox_WorldPosToChunkIndex_nolock(
        lvlbox, pos, ignore_lvlbox_offset
    );
    mutex_Release(_lvlbox_Internal(lvlbox)->m);
    return idx;
}

S3DEXP int spew3d_lvlbox_ExpandToPosition(
        s3d_lvlbox *lvlbox, s3d_pos pos
        ) {
    mutex_Lock(_lvlbox_Internal(lvlbox)->m);
    int result = _spew3d_lvlbox_ExpandToPosition_nolock(
        lvlbox, pos
    );
    mutex_Release(_lvlbox_Internal(lvlbox)->m);
    return result;
}

S3DHID int _spew3d_lvlbox_ExpandToPosition_nolock(
        s3d_lvlbox *lvlbox, s3d_pos pos
        ) {
    s3d_pos localpos = pos;
    localpos.x -= lvlbox->offset.x;
    localpos.y -= lvlbox->offset.y;
    localpos.z -= lvlbox->offset.z;
    int32_t chunk_x = floor((s3dnum_t)localpos.x /
        (((s3dnum_t)LVLBOX_TILE_SIZE) *
        ((s3dnum_t)LVLBOX_CHUNK_SIZE)));
    int32_t chunk_y = floor((s3dnum_t)localpos.y /
        (((s3dnum_t)LVLBOX_TILE_SIZE) *
        ((s3dnum_t)LVLBOX_CHUNK_SIZE)));

    int32_t expand_minus_chunkx = 0;
    s3dnum_t expand_minus_posx = 0;
    int32_t expand_minus_chunky = 0;
    s3dnum_t expand_minus_posy = 0;
    int32_t expand_plus_chunkx = 0;
    int32_t expand_plus_chunky = 0;
    if (chunk_x < 0) {
        expand_minus_posx = (-chunk_x) * (
            ((s3dnum_t)LVLBOX_TILE_SIZE) *
            ((s3dnum_t)LVLBOX_CHUNK_SIZE));
        expand_minus_chunkx = (-chunk_x);
    } else if (chunk_x >= lvlbox->chunk_extent_x) {
        expand_plus_chunkx = (
            chunk_x - lvlbox->chunk_extent_x
        ) + 1;
    }
    uint32_t chunk_extent_y = (
        lvlbox->chunk_count / lvlbox->chunk_extent_x
    );
    assert(chunk_extent_y >= 1);
    if (chunk_y < 0) {
        expand_minus_posy = (-chunk_y) * (
            ((s3dnum_t)LVLBOX_TILE_SIZE) *
            ((s3dnum_t)LVLBOX_CHUNK_SIZE));
        expand_minus_chunky = (-chunk_y);
    } else if (chunk_y >= chunk_extent_y) {
        expand_plus_chunky = (
            chunk_y - chunk_extent_y
        ) + 1;
    }
    if (expand_plus_chunky > 0 || expand_minus_chunkx > 0) {
        int result = _spew3d_lvlbox_ResizeChunksY_nolock(
            lvlbox, chunk_extent_y +
            expand_plus_chunky + expand_minus_chunky
        );
        if (!result) {
            return 0;
        }
    }
    if (expand_plus_chunkx > 0 || expand_minus_chunkx > 0) {
        int result = _spew3d_lvlbox_ResizeChunksY_nolock(
            lvlbox, lvlbox->chunk_extent_x +
            expand_plus_chunkx + expand_minus_chunkx
        );
        if (!result) {
            return 0;
        }
    }

    return 1;
}

struct lvlbox_load_settings {
    int new_if_missing;
    char *new_default_tex;
    int new_default_tex_vfs_flags;
};

S3DHID void *_spew3d_lvlbox_DoMapLoad(
        const char *map_file_path, int map_file_vfs_flags,
        void *extra
        ) {
    assert(extra != NULL);
    struct lvlbox_load_settings *settings = extra;

    char *result = NULL;
    uint64_t result_len;
    if (!spew3d_vfs_FileToBytesWithLimit(
            map_file_path, map_file_vfs_flags,
            10 * 1024 * 1024,
            NULL, &result, &result_len
            )) {
        int _exists = 0;
        if (settings->new_if_missing &&
                spew3d_vfs_Exists(
                map_file_path, map_file_vfs_flags, &_exists, NULL
                ) && !_exists) {
            s3d_lvlbox *lvlbox = spew3d_lvlbox_New(
                settings->new_default_tex,
                settings->new_default_tex_vfs_flags
            );
            free(settings);
            return lvlbox;
        }
        free(settings);
        return NULL;
    }
    assert(result != NULL);

    s3d_lvlbox *lvlbox = spew3d_lvlbox_FromString(
        result, result_len
    );
    if (!result) {
        free(settings);
        return NULL;
    }

    // We reached the end, clean up:
    free(settings);
    return lvlbox;
}

S3DEXP s3d_resourceload_job *spew3d_lvlbox_FromMapFileOrNew(
        const char *map_file_path, int map_file_vfs_flags,
        int new_if_missing,
        const char *new_default_tex, int new_default_tex_vfs_flags
        ) {
    struct lvlbox_load_settings *settings = malloc(sizeof(*settings));
    if (!settings)
        return NULL;
    memset(settings, 0, sizeof(*settings));
    settings->new_if_missing = new_if_missing;
    if (new_default_tex != NULL) {
        settings->new_default_tex = strdup(new_default_tex);
        if (settings->new_default_tex == NULL) {
            free(settings);
            return NULL;
        }
    }
    settings->new_default_tex_vfs_flags = new_default_tex_vfs_flags;
    s3d_resourceload_job *job = s3d_resourceload_NewJobWithCallback(
        map_file_path, RLTYPE_LVLBOX, map_file_vfs_flags,
        _spew3d_lvlbox_DoMapLoad, settings
    );
    return job;
}

S3DEXP s3d_resourceload_job *spew3d_lvlbox_FromMapFile(
        const char *map_file_path, int map_file_vfs_flags
        ) {
    return spew3d_lvlbox_FromMapFileOrNew(
        map_file_path, map_file_vfs_flags, 0, NULL, 0
    );
}

S3DEXP s3d_lvlbox *spew3d_lvlbox_FromMapFileFinalize(
        s3d_resourceload_job *job
        ) {
    s3d_resourceload_result job_output = {0};
    int result = s3d_resourceload_ExtractResult(
        job, &job_output, NULL
    );
    if (!result) {
        s3d_resourceload_DestroyJob(job);
        return NULL;
    }
    s3d_lvlbox *lvlbox = job_output.generic.callback_result;
    s3d_resourceload_DestroyJob(job);
    return lvlbox;
}

struct lvlbox_save_settings {
    s3d_lvlbox *lvlbox;
};

S3DHID void *_spew3d_lvlbox_DoMapSave(
        const char *map_file_path, int map_file_vfs_flags,
        void *extra
        ) {
    assert(extra != NULL);
    struct lvlbox_save_settings *settings = extra;

    uint32_t out_bytes = 0;
    char *serialized = spew3d_lvlbox_ToString(
        settings->lvlbox, &out_bytes
    );
    if (!serialized || out_bytes <= 0) {
        free(settings);
        return NULL;
    }

    SPEW3DVFS_FILE *f = spew3d_vfs_fopen(
        map_file_path, "wb", map_file_vfs_flags
    );
    if (!f) {
        free(settings);
        return NULL;
    }
    size_t written = spew3d_vfs_fwrite(
        serialized, 1, out_bytes, f
    );
    spew3d_vfs_fclose(f);
    if (written < out_bytes) {
        free(settings);
        return NULL;
    }

    // We reached the end, clean up and return success:
    free(settings);
    return (void*)1;
}

S3DEXP s3d_resourceload_job *spew3d_lvlbox_ToMapFile(
        s3d_lvlbox *lvlbox,
        const char *map_file_path, int map_file_vfs_flags
        ) {
    struct lvlbox_save_settings *settings = (
        malloc(sizeof(*settings))
    );
    if (!settings)
        return NULL;
    memset(settings, 0, sizeof(*settings));
    settings->lvlbox = lvlbox;

    s3d_resourceload_job *job = s3d_resourceload_NewJobWithCallback(
        map_file_path, RLTYPE_LVLBOX_STORE, map_file_vfs_flags,
        _spew3d_lvlbox_DoMapSave, settings
    );
    return job;
}

S3DHID static int _lvlbox_FromStr_CheckStr(const char **s,
        uint32_t *slen,
        s3d_lvlbox **lvlbox, const char *value) {
    uint32_t vlen = strlen(value);
    if (*slen < vlen || memcmp(s, value, vlen) != 0)
        return 0;
    *slen += vlen;
    return 1;
}

S3DHID static int _lvlbox_FromStr_CheckSpace(
        const char **s, uint32_t *slen,
        s3d_lvlbox **lvlbox, int optional) {
    if (*slen <= 0) {
        if (optional)
            return 1;
        return 0;
    }
    const char *new_s = *s;
    uint32_t new_slen = *slen;
    uint32_t spaces = 0;
    while (*slen > 0 && (**s == ' ' ||
            **s == '\t' || **s == '\n')) {
        spaces++;
        new_slen--;
        new_s++;
    }
    if (optional || spaces > 0) {
        *s = new_s;
        *slen = new_slen;
        return 1;
    }
    return 0;
}

#define CHECK_STR(x) \
    if (!_lvlbox_FromStr_CheckStr(&s, &slen, &lvlbox, x)) {\
        _spew3d_lvlbox_ActuallyDestroy(lvlbox);\
        return NULL;\
    }
#define CHECK_SPACE(optional) \
    if (!_lvlbox_FromStr_CheckSpace(&s, &slen, &lvlbox, optional != 0)) {\
        _spew3d_lvlbox_ActuallyDestroy(lvlbox);\
        return NULL;\
    }
/*#define CHECK_INT(v) \
    if (!_lvlbox_FromStr_CheckNumber(&s, &slen, &lvlbox, v, 1)) {\
        _spew3d_lvlbox_ActuallyDestroy(lvlbox);\
        return NULL;\
    }*/

S3DEXP s3d_lvlbox *spew3d_lvlbox_FromString(
        const char *s, uint32_t slen
        ) {
    s3d_lvlbox *lvlbox = spew3d_lvlbox_New(NULL, 0);
    if (!lvlbox)
        return NULL;

    const char *s_start = s;

    CHECK_STR("S3DLVLBOX");
    CHECK_SPACE(0);
    CHECK_STR("V");

    return lvlbox;
}

#undef CHECK_STR
#undef CHECK_SPACE

S3DEXP char *spew3d_lvlbox_ToString(
        s3d_lvlbox *lvlbox, uint32_t *out_slen
        ) {
    mutex_Lock(_lvlbox_Internal(lvlbox)->m);

    mutex_Release(_lvlbox_Internal(lvlbox)->m);
    return NULL;
}

S3DEXP s3d_lvlbox *spew3d_lvlbox_New(
        const char *default_tex, int default_tex_vfs_flags
        ) {
    s3d_lvlbox *lvlbox = malloc(sizeof(*lvlbox));
    if (!lvlbox) {
        return NULL;
    }
    memset(lvlbox, 0, sizeof(*lvlbox));

    lvlbox->_internal = malloc(sizeof(s3d_lvlbox_internal));
    if (!lvlbox->_internal) {
        free(lvlbox);
        return NULL;
    }
    memset(lvlbox->_internal, 0, sizeof(s3d_lvlbox_internal));

    _lvlbox_Internal(lvlbox)->m = mutex_Create();
    if (!_lvlbox_Internal(lvlbox)->m) {
        free(lvlbox->_internal);
        free(lvlbox);
        return NULL;
    }
    lvlbox->chunk = malloc(sizeof(*lvlbox->chunk));
    if (!lvlbox->chunk) {
        mutex_Destroy(_lvlbox_Internal(lvlbox)->m);
        free(lvlbox->_internal);
        free(lvlbox);
        return NULL;
    }
    memset(lvlbox->chunk, 0, sizeof(*lvlbox->chunk));
    memset(lvlbox->chunk[0].tile, 0,
        sizeof(s3d_lvlbox_tile) * LVLBOX_CHUNK_SIZE *
        LVLBOX_CHUNK_SIZE);
    lvlbox->chunk_count = 1;
    lvlbox->chunk_extent_x = 1;
    if (default_tex != NULL) {
        s3d_pos pos = {0};
        pos.x = ((s3dnum_t)LVLBOX_CHUNK_SIZE *
            (s3dnum_t)LVLBOX_TILE_SIZE) * 0.5;
        pos.y = ((s3dnum_t)LVLBOX_CHUNK_SIZE *
            (s3dnum_t)LVLBOX_TILE_SIZE) * 0.5;
        if (!_spew3d_lvlbox_SetFloorOrCeilOrWallTextureAt_nolock(
                lvlbox, pos, default_tex, default_tex_vfs_flags,
                0, -1, 0
                )) {
            _spew3d_lvlbox_ActuallyDestroy(lvlbox);
            return NULL;
        }
        #if defined(DEBUG_SPEW3D_LVLBOX)
        printf("spew3d_lvlbox.c: debug: lvlbox %p "
            "created: "
            "chunk_count=%d tile_count=%d default_tex=%s "
            "center_tile_pos=%f/%f\n",
            lvlbox, (int)lvlbox->chunk_count,
            (int)lvlbox->chunk_count * LVLBOX_CHUNK_SIZE *
            LVLBOX_CHUNK_SIZE, default_tex, (double)pos.x,
            (double)pos.y);
        #endif
    } else {
        #if defined(DEBUG_SPEW3D_LVLBOX)
        printf("spew3d_lvlbox.c: debug: lvlbox %p "
            "created: "
            "chunk_count=%d tile_count=%d default_tex=NULL "
            "center_tile_pos=NULL\n",
            lvlbox, (int)lvlbox->chunk_count,
            (int)lvlbox->chunk_count * LVLBOX_CHUNK_SIZE *
            LVLBOX_CHUNK_SIZE);
        #endif
    }
    return lvlbox;
}

S3DEXP int spew3d_lvlbox_SetFloorTextureAt(
        s3d_lvlbox *lvlbox, s3d_pos pos,
        const char *texname, int vfsflags
        ) {
    mutex_Lock(_lvlbox_Internal(lvlbox)->m);
    int result = _spew3d_lvlbox_SetFloorOrCeilOrWallTextureAt_nolock(
        lvlbox, pos, texname, vfsflags, 0, -1, 0
    );
    mutex_Release(_lvlbox_Internal(lvlbox)->m);
    return result;
}

S3DEXP int spew3d_lvlbox_SetCeilingTextureAt(
        s3d_lvlbox *lvlbox, s3d_pos pos,
        const char *texname, int vfsflags
        ) {
    mutex_Lock(_lvlbox_Internal(lvlbox)->m);
    int result = _spew3d_lvlbox_SetFloorOrCeilOrWallTextureAt_nolock(
        lvlbox, pos, texname, vfsflags, 1, -1, 0
    );
    mutex_Release(_lvlbox_Internal(lvlbox)->m);
    return result;
}

S3DHID int _spew3d_lvlbox_SetFloorOrCeilOrWallTextureAt_nolock(
        s3d_lvlbox *lvlbox, s3d_pos pos,
        const char *texname, int vfsflags,
        int to_ceiling, int to_wall_no, int to_wall_top_part
        ) {
    assert(to_ceiling == 0 || to_wall_no < 0);
    if (!_spew3d_lvlbox_ExpandToPosition_nolock(lvlbox, pos)) {
        return 0;
    }
    uint32_t chunk_index, tile_index;
    int32_t segment_no = -1;
    s3d_pos tile_lower_bound;
    s3d_pos pos_offset;

    int result = _spew3d_lvlbox_WorldPosToTilePos_nolock(
        lvlbox, pos, 0, &chunk_index, &tile_index,
        &tile_lower_bound, &pos_offset, &segment_no
    );
    if (!result || tile_index < 0) {
        return 0;
    }
    s3d_lvlbox_tile *tile = &(
        lvlbox->chunk[chunk_index].tile[tile_index]
    );
    char *set_tex_name = NULL;
    char *last_used_name = NULL;
    s3d_texture_t tid = 0;
    if (texname != NULL) {
        set_tex_name = strdup(texname);
        if (!set_tex_name) {
            return 0;
        }
        last_used_name = strdup(texname);
        if (!last_used_name) {
            free(set_tex_name);
            return 0;
        }
        tid = spew3d_texture_FromFile(
            set_tex_name, vfsflags
        );
        if (tid == 0) {
            free(last_used_name);
            free(set_tex_name);
            return 0;
        }
    }
    int reset_height_floor = 0;
    int reset_height_ceiling = 0;
    int apply_to_seg_no = -1;
    if (!tile->occupied) {
        assert(segment_no < 0);
        assert(tile->segment == NULL);
        tile->segment = malloc(sizeof(*tile->segment));
        if (!tile->segment) {
            free(last_used_name);
            free(set_tex_name);
            return 0;
        }
        tile->occupied = 1;
        memset(tile->segment, 0, sizeof(*tile->segment) * 1);
        tile->segment_count = 1;
        apply_to_seg_no = 0;
        reset_height_floor = 1;
        reset_height_ceiling = 1;
    }
    if (apply_to_seg_no < 0) {
        apply_to_seg_no = (
            _spew3d_lvlbox_TileVertPosToSegmentNo_nolock(
                lvlbox, tile, pos.z, 0
            )
        );
        if (apply_to_seg_no < 0) {
            free(last_used_name);
            free(set_tex_name);
            return 0;
        }
    }
    if ((to_wall_no >= 0 || !to_ceiling) &&
            tile->segment[apply_to_seg_no].floor_tex.name == NULL) {
        reset_height_floor = 1;
    }
    if ((to_wall_no >= 0 || to_ceiling) &&
            tile->segment[apply_to_seg_no].ceiling_tex.name == NULL) {
        reset_height_ceiling = 1;
    }

    if (reset_height_floor || reset_height_ceiling) {
        // Set default height of floor by trying to check neighbors:
        int corners_floor_set[4] = {0};
        int corners_ceiling_set[4] = {0};
        int corner = 0;
        while (corner < 4) {
            int neighbor_corners_floor_set = 0;
            s3dnum_t neighbor_corners_floor_z_best = 0;
            int neighbor_corners_ceiling_set = 0;
            s3dnum_t neighbor_corners_ceiling_z_best = 0;
            int shift_x = -2;
            int shift_y = -2;
            while (shift_x <= 0) {
                shift_x += 1;
                shift_y = -2;
                while (shift_y <= 0) {
                    shift_y += 1;
                    if (shift_x == 0 && shift_y == 0)
                        continue;

                    uint32_t neighbor_chunk_index, neighbor_tile_index;
                    int result = _spew3d_lvlbox_GetNeighborTile_nolock(
                        lvlbox, chunk_index, tile_index,
                        shift_x, shift_y,
                        &neighbor_chunk_index,
                        &neighbor_tile_index
                    );
                    if (!result)
                        continue;

                    double floor_height = 0;
                    result = _spew3d_lvlbox_GetNeighborHeightAtCorner_nolock(
                        lvlbox, chunk_index, tile_index, corner, 0,
                        neighbor_chunk_index, neighbor_tile_index,
                        pos.z, &floor_height
                    );
                    if (result) {
                        if (!neighbor_corners_floor_set ||
                                fabs(pos.z - floor_height) <
                                fabs(pos.z -
                                    neighbor_corners_floor_z_best)) {
                            neighbor_corners_floor_z_best =
                                floor_height;
                        }
                        neighbor_corners_floor_set = 1;
                    }
                    double ceiling_height = 0;
                    result = _spew3d_lvlbox_GetNeighborHeightAtCorner_nolock(
                        lvlbox, chunk_index, tile_index, corner, 1,
                        neighbor_chunk_index, neighbor_tile_index,
                        pos.z, &ceiling_height
                    );
                    if (result) {
                        if (!neighbor_corners_ceiling_set ||
                                fabs(pos.z - ceiling_height) <
                                fabs(pos.z -
                                    neighbor_corners_ceiling_z_best)) {
                            neighbor_corners_ceiling_z_best =
                                ceiling_height;
                        }
                        neighbor_corners_floor_set = 1;
                    }
                }
            }
            if (neighbor_corners_floor_set && reset_height_floor) {
                tile->segment[0].floor_z[corner] =
                    neighbor_corners_floor_z_best;
                corners_floor_set[corner] = 1;
            }
            if (neighbor_corners_ceiling_set && reset_height_ceiling) {
                tile->segment[0].ceiling_z[corner] =
                    neighbor_corners_ceiling_z_best;
                corners_ceiling_set[corner] = 1;
            }
            corner++;
        }
        // If any corner had no neighbors, copy from corners that did:
        corner = 0;
        while (corner < 4) {
            if (!corners_floor_set[corner] && reset_height_floor) {
                int foundother = 0;
                int k = 0;
                while (k < 4) {
                    if (corners_floor_set[k]) {
                        foundother = 1;
                        tile->segment[0].floor_z[corner] =
                            tile->segment[0].floor_z[k];
                        break;
                    }
                    k++;
                }
                if (!foundother)
                    tile->segment[0].floor_z[corner] = pos.z - (
                        (s3dnum_t)LVLBOX_DEFAULT_TILE_HEIGHT / 2.0
                    );
            }
            if (!corners_ceiling_set[corner] && reset_height_ceiling) {
                int foundother = 0;
                int k = 0;
                while (k < 4) {
                    if (corners_ceiling_set[k]) {
                        foundother = 1;
                        tile->segment[0].ceiling_z[corner] =
                            tile->segment[0].ceiling_z[k];
                        break;
                    }
                    k++;
                }
                if (!foundother)
                    tile->segment[0].ceiling_z[corner] = (
                        tile->segment[0].floor_z[corner] + (
                            (s3dnum_t)LVLBOX_DEFAULT_TILE_HEIGHT
                        )
                    );
            }
            corner++;
        }
        segment_no = 0;
    }
    assert(segment_no >= 0 && segment_no < tile->segment_count);
    tile->segment[segment_no].cache.is_up_to_date = 0;
    tile->segment[segment_no].cache.flat_normals_set = 0;
    if (to_ceiling) {
        tile->segment[segment_no].ceiling_tex.name = set_tex_name;
        tile->segment[segment_no].ceiling_tex.vfs_flags = vfsflags;
        tile->segment[segment_no].ceiling_tex.id = tid;
        tile->segment[segment_no].ceiling_tex.wrapmode =
            S3D_LVLBOX_TEXWRAP_MODE_DEFAULT;
    } else if (to_wall_no >= 0) {
        assert(to_wall_no >= 0 && to_wall_no < 4);
        const int i = to_wall_no;
        if (!to_wall_top_part) {
            tile->segment[segment_no].wall[i].tex.name = set_tex_name;
            tile->segment[segment_no].wall[i].tex.vfs_flags = vfsflags;
            tile->segment[segment_no].wall[i].tex.id = tid;
            tile->segment[segment_no].wall[i].tex.wrapmode =
                S3D_LVLBOX_TEXWRAP_MODE_DEFAULT;
        } else {
            tile->segment[segment_no].topwall[i].tex.name =
                set_tex_name;
            tile->segment[segment_no].topwall[i].tex.vfs_flags =
                vfsflags;
            tile->segment[segment_no].topwall[i].tex.id = tid;
            tile->segment[segment_no].topwall[i].tex.wrapmode =
                S3D_LVLBOX_TEXWRAP_MODE_DEFAULT;
        }
    } else {
        tile->segment[segment_no].floor_tex.name = set_tex_name;
        tile->segment[segment_no].floor_tex.vfs_flags = vfsflags;
        tile->segment[segment_no].floor_tex.id = tid;
        tile->segment[segment_no].floor_tex.wrapmode =
            S3D_LVLBOX_TEXWRAP_MODE_DEFAULT;
    }
    if (_lvlbox_Internal(lvlbox)->last_used_tex) {
        free(_lvlbox_Internal(lvlbox)->last_used_tex);
        _lvlbox_Internal(lvlbox)->last_used_tex = NULL;
    }
    _lvlbox_Internal(lvlbox)->last_used_tex_vfsflags = vfsflags;
    _lvlbox_Internal(lvlbox)->last_used_tex = last_used_name;
    return 1;
}

S3DHID static inline int spew3d_lvlbox_TransformTilePolygon(
        s3d_lvlbox *lvlbox,
        s3d_lvlbox_tile *tile, int segment_no,
        s3d_lvlbox_tilepolygon *polygon,
        s3d_pos model_pos,
        s3d_rotation model_rot,
        s3d_transform3d_cam_info *cam_info,
        s3d_geometryrenderlightinfo *render_light_info,
        s3d_color scene_ambient,
        s3d_renderpolygon **render_queue,
        uint32_t *render_fill, uint32_t *render_alloc
        ) {
    if (*render_fill >= *render_alloc) {
        return 0;
    }

    double multiplier_vertex_light = 1.0;
    s3d_pos vertex_positions[3];
    s3d_renderpolygon *rqueue = *render_queue;
    uint32_t ralloc = *render_alloc;
    uint32_t rfill = *render_fill;

    // First vertex:
    #if defined(DEBUG_SPEW3D_TRANSFORM3D)
    printf("spew3d_lvlbox.c: debug: lvlbox %p "
        "tile %p segment %d: Transforming floor, "
        "vertex #1/3 input world x,y,z %f,%f,%f "
        "texid=%d\n",
        lvlbox, tile, (int)segment_no,
        (double)polygon->vertex[0].x,
        (double)polygon->vertex[0].y,
        (double)polygon->vertex[0].z,
        (int)polygon->texture
    );
    #endif
    spew3d_math3d_transform3d(
        polygon->vertex[0],
        cam_info, model_pos, model_rot,
        &rqueue[rfill].vertex_pos_pixels[0],
        &rqueue[rfill].vertex_pos[0]
    );
    #if defined(DEBUG_SPEW3D_TRANSFORM3D)
    printf("spew3d_lvlbox.c: debug: lvlbox %p "
        "tile %p segment %d: Transforming floor, "
        "vertex #1/3 output world x,y,z %f,%f,%f\n",
        lvlbox, tile, (int)segment_no,
        (double)rqueue[rfill].vertex_pos_pixels[0].x,
        (double)rqueue[rfill].vertex_pos_pixels[0].y,
        (double)rqueue[rfill].vertex_pos_pixels[0].z);
    #endif
    rqueue[rfill].vertex_texcoord[0] = (
        polygon->texcoord[0]
    );
    rqueue[rfill].vertex_emit[0] = (
        polygon->light_emit[0]
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

    // Second vertex:
    #if defined(DEBUG_SPEW3D_TRANSFORM3D)
    printf("spew3d_lvlbox.c: debug: lvlbox %p "
        "tile %p segment %d: Transforming floor, "
        "vertex #2/3 input world x,y,z %f,%f,%f\n",
        lvlbox, tile, (int)segment_no,
        (double)polygon->vertex[1].x,
        (double)polygon->vertex[1].y,
        (double)polygon->vertex[1].z);
    #endif
    spew3d_math3d_transform3d(
        polygon->vertex[1],
        cam_info, model_pos, model_rot,
        &rqueue[rfill].vertex_pos_pixels[1],
        &rqueue[rfill].vertex_pos[1]
    );
    #if defined(DEBUG_SPEW3D_TRANSFORM3D)
    printf("spew3d_lvlbox.c: debug: lvlbox %p "
        "tile %p segment %d: Transforming floor, "
        "vertex #2/3 output world x,y,z %f,%f,%f\n",
        lvlbox, tile, (int)segment_no,
        (double)rqueue[rfill].vertex_pos_pixels[1].x,
        (double)rqueue[rfill].vertex_pos_pixels[1].y,
        (double)rqueue[rfill].vertex_pos_pixels[1].z);
    #endif
    rqueue[rfill].vertex_texcoord[1] = (
        polygon->texcoord[1]
    );
    rqueue[rfill].vertex_emit[1] = (
        polygon->light_emit[1]
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

    // Third vertex:
    #if defined(DEBUG_SPEW3D_TRANSFORM3D)
    printf("spew3d_lvlbox.c: debug: lvlbox %p "
        "tile %p segment %d: Transforming floor, "
        "vertex #3/3 input world x,y,z %f,%f,%f\n",
        lvlbox, tile, (int)segment_no,
        (double)polygon->vertex[2].x,
        (double)polygon->vertex[2].y,
        (double)polygon->vertex[2].z);
    #endif
    spew3d_math3d_transform3d(
        polygon->vertex[2],
        cam_info, model_pos, model_rot,
        &rqueue[rfill].vertex_pos_pixels[2],
        &rqueue[rfill].vertex_pos[2]
    );
    #if defined(DEBUG_SPEW3D_TRANSFORM3D)
    printf("spew3d_lvlbox.c: debug: lvlbox %p "
        "tile %p segment %d: Transforming floor, "
        "vertex #3/3 output world x,y,z %f,%f,%f\n",
        lvlbox, tile, (int)segment_no,
        (double)rqueue[rfill].vertex_pos_pixels[2].x,
        (double)rqueue[rfill].vertex_pos_pixels[2].y,
        (double)rqueue[rfill].vertex_pos_pixels[2].z);
    #endif
    rqueue[rfill].vertex_texcoord[2] = (
        polygon->texcoord[2]
    );
    rqueue[rfill].vertex_emit[2] = (
        polygon->light_emit[2]
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

    // Set texture and material:
    rqueue[rfill].polygon_texture = (
        polygon->texture
    );
    rqueue[rfill].polygon_material = (
        polygon->material
    );

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
        // No rfill++ here since we're abandoning this slot.
        return 1;
    }

    // Copy over normals:
    rqueue[rfill].vertex_normal[0] = polygon->normal[0];
    rqueue[rfill].vertex_normal[1] = polygon->normal[1];
    rqueue[rfill].vertex_normal[2] = polygon->normal[1];

    rfill++;
    *render_fill = rfill;
    return 1;
}

#define LVLBOX_TRANSFORM_QUEUEGROW(x) \
    if (rfill + (uint32_t)x > ralloc) {\
        uint32_t newalloc = (\
            rfill + (uint32_t)x + 1 + 6\
        ) * 2;\
        s3d_renderpolygon *newqueue = realloc(\
            rqueue, sizeof(*newqueue) * newalloc\
        );\
        if (!newqueue)\
            return 0;\
        rqueue = newqueue;\
        ralloc = newalloc;\
        *render_queue = rqueue;\
        *render_alloc = ralloc;\
    }

S3DEXP int spew3d_lvlbox_Transform(
        s3d_lvlbox *lvlbox,
        s3d_pos *model_pos,
        s3d_rotation *model_rotation,
        s3d_transform3d_cam_info *cam_info,
        s3d_geometryrenderlightinfo *render_light_info,
        s3d_renderpolygon **render_queue,
        uint32_t *render_fill, uint32_t *render_alloc
        ) {
    assert(render_light_info->dynlight_mode !=
        DLRD_INVALID);

    s3d_pos geometry_shift = lvlbox->offset;
    s3d_pos effective_model_pos = {0};
    if (model_pos != NULL)
        memcpy(&effective_model_pos, model_pos,
            sizeof(*model_pos));
    s3d_rotation effective_model_rot = {0};
    if (model_rotation != NULL)
        memcpy(&effective_model_rot, model_rotation,
            sizeof(*model_rotation));

    s3d_renderpolygon *rqueue = *render_queue;
    uint32_t ralloc = *render_alloc;
    uint32_t rfill = *render_fill;
    LVLBOX_TRANSFORM_QUEUEGROW(10);

    assert(ralloc > 0 && rqueue != NULL);

    s3d_color scene_ambient = render_light_info->ambient_emit;
    if (render_light_info->dynlight_mode == DLRD_UNLIT) {
        scene_ambient.red = 1.0;
        scene_ambient.green = 1.0;
        scene_ambient.blue = 1.0;
    }

    uint32_t i = 0;
    while (i < lvlbox->chunk_count) {
        uint32_t k = 0;
        while (k < (uint32_t)LVLBOX_CHUNK_SIZE *
                (uint32_t)LVLBOX_CHUNK_SIZE) {
            s3d_lvlbox_tile *tile = &lvlbox->chunk[i].tile[k];
            if (!tile->occupied) {
                k++;
                continue;
            }
            //printf("RENDERING CHUNK %d TILE %d\n",
            //    (int)i, (int)k);
            if (!spew3d_lvlbox_TryUpdateTileCache_nolock(
                    lvlbox, i, k
                    )) {
                #if defined(DEBUG_SPEW3D_LVLBOX)
                printf("spew3d_lvlbox.c: debug: lvlbox %p "
                    "chunk %d tile %d: Failed to update tile cache.\n",
                    lvlbox, (int)i, (int)k);
                #endif
                k++;
                continue;
            }
            uint32_t i2 = 0;
            while (i2 < tile->segment_count) {
                assert(tile->segment[i2].cache.is_up_to_date);
                LVLBOX_TRANSFORM_QUEUEGROW(
                    tile->segment[i2].cache.cached_floor_polycount
                );
                uint32_t i3 = 0;
                while (i3 < tile->segment[i2].cache.
                        cached_floor_polycount) {
                    *render_fill = rfill;
                    *render_alloc = ralloc;
                    int result = spew3d_lvlbox_TransformTilePolygon(
                        lvlbox, tile, i2,
                        &tile->segment[i2].cache.cached_floor[i3],
                        effective_model_pos, effective_model_rot,
                        cam_info, render_light_info, scene_ambient,
                        render_queue, render_fill, render_alloc
                    );
                    if (!result) {
                        #if defined(DEBUG_SPEW3D_LVLBOX)
                        printf("spew3d_lvlbox.c: debug: lvlbox %p "
                            "chunk %d tile %d floor polygon %d: "
                            "Somehow failed to transform polygon.\n",
                            lvlbox, (int)i, (int)k, (int)i3);
                        #endif
                    }
                    rfill = *render_fill;
                    i3++;
                }
                i2++;
            }
            k++;
        }
        i++;
    }

    *render_fill = rfill;
    return 1;
}

S3DEXP int spew3d_lvlbox_HoriAngleToCornerIndex(
        s3dnum_t angle
        ) {
    angle = spew3d_math3d_normalizeangle(angle);
    if (angle > 0) {
        if (angle < 90) {
            return 0;
        } else {
            return 1;
        }
    } else {
        if (angle > -90) {
            return 3;
        } else {
            return 2;
        }
    }
}

S3DEXP int _spew3d_lvlbox_GetTileClosestCornerNearWorldPos_nolock(
        s3d_lvlbox *lvlbox, uint32_t chunk_index,
        uint32_t tile_index, int segment_no,
        s3d_pos pos, int ignore_z, int at_ceiling,
        s3d_pos *out_corner_pos
        ) {
    if (chunk_index < 0 || chunk_index >= lvlbox->chunk_count)
        return 0;

    s3d_pos closest_corner_pos;
    int closest_corner = -1;
    s3dnum_t closest_corner_dist = 0;
    int corner = 0;
    while (corner < 4) {
        s3d_pos corner_pos = {0};
        
        int result = _spew3d_lvlbox_CornerPosToWorldPos_nolock(
            lvlbox, chunk_index, tile_index, segment_no,
            corner, at_ceiling, &corner_pos
        );
        if (!result) {
            corner++;
            continue;
        }

        s3d_pos cmp_pos = corner_pos;
        ignore_z = 1; // Hack: always ignore Z component for now.
        if (ignore_z) {
            cmp_pos.z = pos.z;
        }
        s3dnum_t dist_to_corner = spew3d_math3d_dist(
            &cmp_pos, &pos
        );
        if (closest_corner < 0 ||
                dist_to_corner < closest_corner_dist) {
            closest_corner = corner;
            closest_corner_dist = dist_to_corner;
            closest_corner_pos = corner_pos;
        }
        corner++;
    }
    if (closest_corner < 0)
        return 0;
    assert(closest_corner >= 0);
    if (out_corner_pos) *out_corner_pos = closest_corner_pos;
    return closest_corner;
}

S3DEXP int spew3d_lvlbox_GetTileClosestCornerNearWorldPos(
        s3d_lvlbox *lvlbox, uint32_t chunk_index,
        uint32_t tile_index, int segment_no,
        s3d_pos pos, int ignore_z, int at_ceiling,
        s3d_pos *out_corner_pos
        ) {
    mutex_Lock(_lvlbox_Internal(lvlbox)->m);
    int result = _spew3d_lvlbox_GetTileClosestCornerNearWorldPos_nolock(
        lvlbox, chunk_index, tile_index, segment_no, pos,
        ignore_z, at_ceiling, out_corner_pos
    );
    mutex_Release(_lvlbox_Internal(lvlbox)->m);
    return result;
}

S3DEXP int spew3d_lvlbox_InteractPosDirToTileCorner(
        s3d_lvlbox *lvlbox, s3d_pos interact_pos,
        s3d_rotation interact_rot,
        int32_t *chunk_index, int32_t *tile_index,
        int32_t *chunk_x, int32_t *chunk_y,
        int32_t *tile_x, int32_t *tile_y,
        int32_t *segment_no, int *corner
        ) {
    mutex_Lock(_lvlbox_Internal(lvlbox)->m);
    int result = _spew3d_lvlbox_InteractPosDirToTileCorner_nolock(
        lvlbox, interact_pos, interact_rot, chunk_index,
        tile_index, chunk_x, chunk_y, tile_x,
        tile_y, segment_no, corner
    );
    mutex_Release(_lvlbox_Internal(lvlbox)->m);
    return result;
}

S3DHID int _spew3d_lvlbox_InteractPosDirToTileCorner_nolock(
        s3d_lvlbox *lvlbox, s3d_pos interact_pos,
        s3d_rotation interact_rot,
        int32_t *chunk_index, int32_t *tile_index,
        int32_t *chunk_x, int32_t *chunk_y,
        int32_t *tile_x, int32_t *tile_y,
        int32_t *segment_no, int *corner
        ) {
    spew3d_math3d_normalizerot(&interact_rot);

    s3d_pos advance_in_dir = {0};
    advance_in_dir.x = LVLBOX_TILE_SIZE * 0.3;
    s3d_rotation r = {0};
    r.hori = interact_rot.hori;
    spew3d_math3d_rotate(&advance_in_dir, &r);

    s3d_pos query_pos_low = interact_pos;
    uint32_t query_pos_low_chunk_index,
        query_pos_low_tile_index;
    s3d_pos query_pos_low_offset;
    int32_t query_pos_low_segment_no;
    int result = _spew3d_lvlbox_WorldPosToTilePos_nolock(
        lvlbox, interact_pos, 0,
        &query_pos_low_chunk_index,
        &query_pos_low_tile_index,
        NULL,
        &query_pos_low_offset,
        &query_pos_low_segment_no
    );
    if (result &&
            query_pos_low_offset.z < LVLBOX_TILE_SIZE * 0.5) {
        int icorner = spew3d_lvlbox_HoriAngleToCornerIndex(
            interact_rot.hori
        );
        *chunk_index = query_pos_low_chunk_index;
        *tile_index = query_pos_low_tile_index;
        *segment_no = query_pos_low_segment_no;
        *corner = icorner;
        return 1;
    } else if (!result) {
        // We're out of bounds.
        // Don't try to guess ahead for a tile, so it's easier
        // to target out of bounds for adding geometry.
        return 0;
    }

    s3d_pos query_pos_high = interact_pos;
    query_pos_high.x += advance_in_dir.x;
    query_pos_high.y += advance_in_dir.y;
    query_pos_high.z += advance_in_dir.z;
    uint32_t query_pos_high_chunk_index,
        query_pos_high_tile_index;
    s3d_pos query_pos_high_offset;
    int32_t query_pos_high_segment_no;
    int result2 = _spew3d_lvlbox_WorldPosToTilePos_nolock(
        lvlbox, query_pos_high, 0,
        &query_pos_high_chunk_index,
        &query_pos_high_tile_index,
        NULL,
        &query_pos_high_offset,
        &query_pos_high_segment_no
    );
    if (!result2) {
        if (!result)
            return 0;
        int icorner = spew3d_lvlbox_HoriAngleToCornerIndex(
            interact_rot.hori
        );
        *chunk_index = query_pos_low_chunk_index;
        *tile_index = query_pos_low_tile_index;
        *segment_no = query_pos_low_segment_no;
        *corner = icorner;
        return 1;
    }

    s3d_pos lookat_advance = {0};
    lookat_advance.x = LVLBOX_TILE_SIZE;
    spew3d_math3d_rotate(&lookat_advance, &interact_rot);
    s3d_pos lookat_pos = interact_pos;
    spew3d_math3d_add(&lookat_pos, &lookat_advance);

    int icorner = _spew3d_lvlbox_GetTileClosestCornerNearWorldPos_nolock(
        lvlbox, query_pos_high_chunk_index,
        query_pos_high_tile_index,
        query_pos_high_segment_no,
        lookat_pos, 1, interact_rot.verti > 0, NULL
    );
    *corner = icorner;
    *chunk_index = query_pos_high_chunk_index;
    *tile_index = query_pos_high_tile_index;
    *segment_no = query_pos_high_segment_no;
    return 1;
}

S3DEXP int spew3d_lvlbox_CornerPosToWorldPos(
        s3d_lvlbox *lvlbox, uint32_t _chunk_index,
        uint32_t _tile_index, int32_t _segment_no,
        int corner, int at_ceiling, s3d_pos *out_pos
        ) {
    mutex_Lock(_lvlbox_Internal(lvlbox)->m);
    int result = _spew3d_lvlbox_CornerPosToWorldPos_nolock(
        lvlbox, _chunk_index, _tile_index, _segment_no,
        corner, at_ceiling, out_pos
    );
    mutex_Release(_lvlbox_Internal(lvlbox)->m);
    return result;
}

S3DHID int _spew3d_lvlbox_CornerPosToWorldPos_nolock(
        s3d_lvlbox *lvlbox, uint32_t _chunk_index,
        uint32_t _tile_index, int32_t _segment_no,
        int corner, int at_ceiling, s3d_pos *out_pos
        ) {
    if (_chunk_index < 0 || _chunk_index >= lvlbox->chunk_count)
        return 0;
    if (_tile_index < 0 || _tile_index >= LVLBOX_CHUNK_SIZE *
            LVLBOX_CHUNK_SIZE)
        return 0;
    uint32_t _chunk_x, _chunk_y, _tile_x, _tile_y;
    _spew3d_lvlbox_TileAndChunkIndexToPos_nolock(
        lvlbox, _chunk_index, _tile_index,
        &_chunk_x, &_chunk_x, &_tile_x, &_tile_x,
        NULL
    );

    s3d_pos corner_pos = {0};
    s3d_pos tile_pos = lvlbox->offset;
    tile_pos.x += _chunk_x * (s3dnum_t)LVLBOX_CHUNK_SIZE *
        (s3dnum_t)LVLBOX_TILE_SIZE +
        _tile_x * (s3dnum_t)LVLBOX_TILE_SIZE;
    tile_pos.y += _chunk_y * (s3dnum_t)LVLBOX_CHUNK_SIZE *
        (s3dnum_t)LVLBOX_TILE_SIZE +
        _tile_y * (s3dnum_t)LVLBOX_TILE_SIZE;
    corner_pos.x = tile_pos.x;
    corner_pos.y = tile_pos.y;
    if (corner == 0) {
        corner_pos.x += (s3dnum_t)LVLBOX_TILE_SIZE;
        corner_pos.y += (s3dnum_t)LVLBOX_TILE_SIZE;
    } else if (corner == 1) {
        corner_pos.y += (s3dnum_t)LVLBOX_TILE_SIZE;
    } else if (corner == 3) {
        corner_pos.x += (s3dnum_t)LVLBOX_TILE_SIZE;
    }
    if (_segment_no >= 0 &&
            lvlbox->chunk[_chunk_index].
            tile[_tile_index].occupied &&
            _segment_no < lvlbox->chunk[_chunk_index].
            tile[_tile_index].segment_count) {
        if (!at_ceiling) {
            corner_pos.z = lvlbox->chunk[_chunk_index].
                tile[_tile_index].segment[_segment_no].
                    floor_z[corner];
        } else {
            corner_pos.z = lvlbox->chunk[_chunk_index].
                tile[_tile_index].segment[_segment_no].
                    ceiling_z[corner];
        }
        corner_pos.z += lvlbox->offset.z;
    } else {
        corner_pos.z = lvlbox->offset.z;
    }
    if (out_pos) {
        *out_pos = corner_pos;
    }
    return 1;
}

S3DEXP int spew3d_lvlbox_InteractPosDirToTileCornerOrWall(
        s3d_lvlbox *lvlbox, s3d_pos interact_pos,
        s3d_rotation interact_rot,
        int32_t *out_chunk_index, int32_t *out_tile_index,
        int32_t *out_chunk_x, int32_t *out_chunk_y,
        int32_t *out_tile_x, int32_t *out_tile_y,
        int32_t *out_segment_no, int *out_corner_no,
        int *out_wall_no
        ) {
    mutex_Lock(_lvlbox_Internal(lvlbox)->m);
    int result = _spew3d_lvlbox_InteractPosDirToTileCornerOrWall_nolock(
        lvlbox, interact_pos, interact_rot,
        out_chunk_index, out_tile_index,
        out_chunk_x, out_chunk_y, out_tile_x, out_tile_y,
        out_segment_no, out_corner_no, out_wall_no
    );
    mutex_Release(_lvlbox_Internal(lvlbox)->m);
    return result;
}

S3DHID int _spew3d_lvlbox_InteractPosDirToTileCornerOrWall_nolock(
        s3d_lvlbox *lvlbox, s3d_pos interact_pos,
        s3d_rotation interact_rot,
        int32_t *out_chunk_index, int32_t *out_tile_index,
        int32_t *out_chunk_x, int32_t *out_chunk_y,
        int32_t *out_tile_x, int32_t *out_tile_y,
        int32_t *out_segment_no, int *out_corner_no,
        int *out_wall_no
        ) {
    int32_t _chunk_index, _tile_index;
    int32_t _chunk_x, _chunk_y, _tile_x, _tile_y, _segment_no;
    int _corner;
    int result = _spew3d_lvlbox_InteractPosDirToTileCorner_nolock(
        lvlbox, interact_pos, interact_rot,
        &_chunk_index, &_tile_index, &_chunk_x, &_chunk_y,
        &_tile_x, &_tile_y, &_segment_no, &_corner
    );
    if (!result)
        return 0;

    s3d_pos corner_pos = {0};
    result = _spew3d_lvlbox_CornerPosToWorldPos_nolock(
        lvlbox, _chunk_index, _tile_index, _segment_no,
        _corner, interact_rot.verti > 0, &corner_pos
    );
    int target_wall = 0;
    if (result) {
        s3d_point hori_dir_to_corner;
        hori_dir_to_corner.x = (
            corner_pos.x - interact_pos.x
        );
        hori_dir_to_corner.y = (
            corner_pos.y - interact_pos.y
        );
        double hori_dist_to_corner = spew3d_math2d_len(
            &hori_dir_to_corner
        );
        double verti_dist_to_corner = fabs(
            corner_pos.z - interact_pos.z
        );
        if (verti_dist_to_corner > hori_dist_to_corner) {
            target_wall = 1;
        }
    }

    if (out_chunk_index)
        *out_chunk_index = _chunk_index;
    if (out_tile_index)
        *out_tile_index = _tile_index;
    if (out_chunk_x)
        *out_chunk_x = _chunk_x;
    if (out_chunk_y)
        *out_chunk_y = _chunk_y;
    if (out_tile_x)
        *out_tile_x = _tile_x;
    if (out_tile_y)
        *out_tile_y = _tile_y;
    if (out_segment_no)
        *out_segment_no = _segment_no;
    if (target_wall) {
        if (out_corner_no)
            *out_corner_no = -1;
        double hori_angle = spew3d_math3d_normalizeangle(
            interact_rot.hori
        );
        int wall_no = 0;
        if (fabs(hori_angle) > 45) {
            if (fabs(hori_angle) > 135) {
                wall_no = 2;
            } else if (hori_angle > 0) {
                wall_no = 1;
            } else {
                assert(hori_angle < 0);
                wall_no = 3;
            }
        }
        if (out_wall_no)
            *out_wall_no = wall_no;
    } else {
        if (out_corner_no) {
            if (result)
                *out_corner_no = _corner;
            else
                *out_corner_no = -1;
        }
        if (out_wall_no)
            *out_wall_no = -1;
    }
    return 1;
}

S3DHID int spew3d_lvlbox_edit_PaintLastUsedTextureEx(
        s3d_lvlbox *lvlbox, s3d_pos paint_pos,
        s3d_rotation paint_aim, int erase,
        int topwallmodifier
        ) {
    mutex_Lock(_lvlbox_Internal(lvlbox)->m);

    const char *paint_name = "grass01.png";
    int paint_vfsflags = 0;
    if (_lvlbox_Internal(lvlbox)->last_used_tex) {
        paint_name = _lvlbox_Internal(lvlbox)->last_used_tex;
        paint_vfsflags = _lvlbox_Internal(lvlbox)->
            last_used_tex_vfsflags;
    }

    int32_t chunk_index, tile_index, segment_no;
    int32_t chunk_x, chunk_y, tile_x, tile_y;
    int corner_no, wall_no;
    int result = _spew3d_lvlbox_InteractPosDirToTileCornerOrWall_nolock(
        lvlbox, paint_pos, paint_aim,
        &chunk_index, &tile_index, &chunk_x, &chunk_y,
        &tile_x, &tile_y, &segment_no, &corner_no, &wall_no
    );
    if (!result) {
        #if defined(DEBUG_SPEW3D_LVLBOX)
        printf("spew3d_lvlbox.c: debug: lvlbox %p "
            "spew3d_lvlbox_edit_PaintLastUsedTexture(): "
            "Not aiming at anything, with "
            "paint_pos x/y/z=%f/%f/%f aim.\n",
            lvlbox, (double)paint_pos.x, (double)paint_pos.y,
            (double)paint_pos.z);
        #endif
        mutex_Release(_lvlbox_Internal(lvlbox)->m);
        return 1;
    }
    assert((wall_no >= 0 || corner_no >= 0) &&
        (wall_no < 0 || corner_no < 0));
    #if defined(DEBUG_SPEW3D_LVLBOX)
    printf("spew3d_lvlbox.c: debug: lvlbox %p "
        "spew3d_lvlbox_edit_PaintLastUsedTexture(): "
        "Aiming chunk_index=%d tile_index=%d "
        "segment_no=%d corner=%d wall=%d, with "
        "paint_pos x/y/z=%f/%f/%f aim.\n",
        lvlbox,
        (int)chunk_index, (int)tile_index, (int)segment_no,
        (int)corner_no, (int)wall_no,
        (double)paint_pos.x, (double)paint_pos.y,
        (double)paint_pos.z);
    #endif

    result = _spew3d_lvlbox_SetFloorOrCeilOrWallTextureAt_nolock(
        lvlbox, paint_pos, (erase ? NULL : paint_name),
        paint_vfsflags,
        (paint_aim.verti > 0 && wall_no < 0),
        wall_no, (wall_no < 0 ? 0 : topwallmodifier)
    );
    mutex_Release(_lvlbox_Internal(lvlbox)->m);
    return result;
}

S3DEXP int spew3d_lvlbox_edit_PaintLastUsedTexture(
        s3d_lvlbox *lvlbox, s3d_pos paint_pos,
        s3d_rotation paint_aim, int topwallmodifier
        ) {
    return spew3d_lvlbox_edit_PaintLastUsedTextureEx(
        lvlbox, paint_pos, paint_aim, 0, topwallmodifier
    );
}

S3DEXP int spew3d_lvlbox_edit_EraseTexture(
        s3d_lvlbox *lvlbox, s3d_pos paint_pos,
        s3d_rotation paint_aim
        ) {
    return spew3d_lvlbox_edit_PaintLastUsedTextureEx(
        lvlbox, paint_pos, paint_aim, 1, 0
    );
}

S3DEXP int spew3d_lvlbox_edit_DragFocusedTileCorner(
        s3d_lvlbox *lvlbox, s3d_pos drag_pos,
        s3d_rotation drag_aim, double drag_z,
        int dragconnected
        ) {
    mutex_Lock(_lvlbox_Internal(lvlbox)->m);

    spew3d_math3d_normalizerot(&drag_aim);
    int drag_at_ceiling = (drag_aim.verti > 0);

    int32_t chunk_index, tile_index;
    int32_t segment_no;
    int32_t chunk_x, chunk_y, tile_x, tile_y;
    int corner;
    int result = _spew3d_lvlbox_InteractPosDirToTileCorner_nolock(
        lvlbox, drag_pos, drag_aim,
        &chunk_index, &tile_index, &chunk_x, &chunk_y,
        &tile_x, &tile_y, &segment_no, &corner
    );
    if (!result) {
        #if defined(DEBUG_SPEW3D_LVLBOX)
        printf("spew3d_lvlbox.c: "
            "debug: lvlbox %p "
            "spew3d_lvlbox_edit_DragFocusedTileCorner(): "
            "Not aiming at anything, with "
            "drag_pos x/y/z=%f/%f/%f aim.\n",
            lvlbox, (double)drag_pos.x, (double)drag_pos.y,
            (double)drag_pos.z);
        #endif
        mutex_Release(_lvlbox_Internal(lvlbox)->m);
        return 1;
    }
    #if defined(DEBUG_SPEW3D_LVLBOX)
    printf("spew3d_lvlbox.c: "
        "debug: lvlbox %p "
        "spew3d_lvlbox_edit_DragFocusedTileCorner(): "
        "Aiming chunk_index=%d tile_index=%d "
        "segment_no=%d corner=%d, with "
        "drag_pos x/y/z=%f/%f/%f aim.\n",
        lvlbox,
        (int)chunk_index, (int)tile_index, (int)segment_no,
        (int)corner, (double)drag_pos.x, (double)drag_pos.y,
        (double)drag_pos.z);
    #endif

    if (segment_no < 0) {
        mutex_Release(_lvlbox_Internal(lvlbox)->m);
        return 1;
    }
    s3d_lvlbox_tile *tile = (
        &lvlbox->chunk[chunk_index].tile[tile_index]
    );
    int segment_above = segment_no + 1;
    if (segment_above >= tile->segment_count)
        segment_above = -1;
    int segment_below = segment_no - 1;
    if (segment_below < 0)
        segment_below = -1;
    double pre_drag_z = (
        (drag_at_ceiling != 0 ?
        tile->segment[segment_no].ceiling_z[corner] :
        tile->segment[segment_no].floor_z[corner])
    );
    double limit_z_below = fmin(
        pre_drag_z, pre_drag_z + drag_z
    );
    double limit_z_above = fmax(
        pre_drag_z, pre_drag_z + drag_z
    );
    if (segment_below >= 0) {
        int k = 0;
        while (k < 4) {
            limit_z_below = fmax(
                limit_z_below,
                tile->segment[segment_below].ceiling_z[k]
            );
            k++;
        }
    }
    if (segment_above >= 0) {
        int k = 0;
        while (k < 4) {
            limit_z_above = fmin(
                limit_z_above,
                tile->segment[segment_above].floor_z[k]
            );
            k++;
        }
    }
    double result_drag_z = fmax(
        limit_z_below, fmin(limit_z_above,
            pre_drag_z + drag_z));
    if (dragconnected) {
        int shift_x = -2;
        while (shift_x <= 0) {
            shift_x++;
            int shift_y = -2;
            while (shift_y <= 0) {
                shift_y++;
                uint32_t neighbor_chunk_index;
                uint32_t neighbor_tile_index;
                int result = _spew3d_lvlbox_GetNeighborTile_nolock(
                    lvlbox, chunk_index, tile_index,
                    shift_x, shift_y,
                    &neighbor_chunk_index, &neighbor_tile_index
                );
                if (!result)
                    continue;
                int neighbor_corner;
                result = _spew3d_lvlbox_GetNeighboringCorner_nolock(
                    lvlbox, chunk_index, tile_index, corner,
                    neighbor_chunk_index, neighbor_tile_index,
                    &neighbor_corner
                );
                if (!result)
                    continue;
                s3d_lvlbox_tile *neighbor_tile = (
                    &lvlbox->chunk[neighbor_chunk_index].
                        tile[neighbor_tile_index]
                );
                if (!neighbor_tile->occupied)
                    continue;
                assert(neighbor_tile->segment_count > 0);
                int n_seg = 0;
                while (n_seg < neighbor_tile->segment_count) {
                    if (drag_at_ceiling && fabs(neighbor_tile->
                            segment[n_seg].ceiling_z[neighbor_corner] -
                            pre_drag_z) < 0.001) {
                        neighbor_tile->
                            segment[n_seg].ceiling_z[neighbor_corner] =
                                result_drag_z;
                        neighbor_tile->segment[n_seg].
                            cache.is_up_to_date = 0;
                        neighbor_tile->segment[n_seg].
                            cache.flat_normals_set = 0;
                    } else if (!drag_at_ceiling && fabs(neighbor_tile->
                            segment[n_seg].floor_z[neighbor_corner] -
                            pre_drag_z) < 0.001) {
                        neighbor_tile->
                            segment[n_seg].floor_z[neighbor_corner] =
                                result_drag_z;
                        neighbor_tile->segment[n_seg].
                            cache.is_up_to_date = 0;
                        neighbor_tile->segment[n_seg].
                            cache.flat_normals_set = 0;
                    }
                    n_seg++;
                }
            }
        }
    }
    if (drag_at_ceiling) {
        tile->segment[segment_no].ceiling_z[corner] = result_drag_z;
    } else {
        tile->segment[segment_no].floor_z[corner] = result_drag_z;
    }
    #if defined(DEBUG_SPEW3D_LVLBOX)
    printf("spew3d_lvlbox.c: "
        "debug: lvlbox %p "
        "spew3d_lvlbox_edit_DragFocusedTileCorner(): "
        "Set chunk_index=%d tile_index=%d "
        "segment_no=%d corner=%d %s_z "
        "from %f to %f as new height. Limiting "
        "height between %f and %f from neighbors.\n",
        lvlbox,
        (int)chunk_index, (int)tile_index, (int)segment_no,
        (int)corner, (drag_at_ceiling ? "ceiling" : "floor"),
        pre_drag_z, result_drag_z,
        limit_z_below,
        limit_z_above);
    #endif
    tile->segment[segment_no].cache.is_up_to_date = 0;
    tile->segment[segment_no].cache.flat_normals_set = 0;
    mutex_Release(_lvlbox_Internal(lvlbox)->m);
    return result;
}

#undef LVLBOX_TRANSFORM_QUEUEGROW

#endif  // SPEW3D_IMPLEMENTATION

