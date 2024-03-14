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

typedef struct s3d_lvlbox s3d_lvlbox;

typedef struct s3d_resourceload_job s3d_resourceload_job;

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
    int hori_fence_targeted = -1;
    s3dnum_t fence_max_z;
    s3dnum_t fence_min_z;
    if (tile->segment[segment_no].hori_fence_count > 0) {
        int best_idx = -1;
        double best_dist = 0;
        s3dnum_t best_z;
        int k = 0;
        while (k < tile->segment[segment_no].
                hori_fence_count) {
            if (drag_aim.verti > 0 &&
                    tile->segment[segment_no].
                    hori_fence_z[k] > drag_pos.z &&
                    (best_idx < 0 || fabs(
                    tile->segment[segment_no].
                    hori_fence_z[k] - drag_pos.z) <
                    best_dist)) {
                best_dist = fabs(
                    tile->segment[segment_no].
                    hori_fence_z[k] - drag_pos.z
                );
                best_z = tile->segment[segment_no].
                    hori_fence_z[k];
                best_idx = k;
                break;
            } else if (drag_aim.verti < 0 &&
                    tile->segment[segment_no].
                    hori_fence_z[k] < drag_pos.z &&
                    (best_idx < 0 || fabs(
                    tile->segment[segment_no].
                    hori_fence_z[k] - drag_pos.z) <
                    best_dist)) {
                best_dist = fabs(
                    tile->segment[segment_no].
                    hori_fence_z[k] - drag_pos.z
                );
                best_z = tile->segment[segment_no].
                    hori_fence_z[k];
                best_idx = k;
                break;
            }
            k++;
        }
        if (best_idx >= 0) {
            double corner_z;
            if (drag_aim.verti > 0)
                corner_z = tile->segment[segment_no].
                    ceiling_z[corner];
            else
                corner_z = tile->segment[segment_no].
                    floor_z[corner];
            if ((drag_aim.verti > 0 && corner_z > best_z) ||
                    (drag_aim.verti < 0 &&
                    corner_z < best_z)) {
                hori_fence_targeted = best_idx;
            }
            fence_max_z = tile->segment[segment_no].
                ceiling_z[0] - min_vertical_spacing * 0.1;
            fence_min_z = tile->segment[segment_no].
                floor_z[0] + min_vertical_spacing * 0.1;
            int k = 1;
            while (k < 4) {
                fence_max_z = fmin(
                    fence_max_z,
                    tile->segment[segment_no].ceiling_z[k]
                );
                fence_min_z = fmax(
                    fence_min_z,
                    tile->segment[segment_no].floor_z[k]
                );
                k++;
            }
            assert(fence_min_z <= fence_max_z);
        }
    }
    if (hori_fence_targeted) {
        double pre_drag_z = tile->segment[segment_no].hori_fence_z[
            hori_fence_targeted
        ];
        double post_drag_z = fmax(fence_min_z,
            fmin(fence_min_z, pre_drag_z + drag_z));
        tile->segment[segment_no].hori_fence_z[
            hori_fence_targeted
        ] = post_drag_z;
        tile->segment[segment_no].cache.is_up_to_date = 0;
        tile->segment[segment_no].cache.flat_normals_set = 0;
        mutex_Release(_lvlbox_Internal(lvlbox)->m);
        return 1;
    }
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
                        _spew3d_lvlbox_InvalidateTileWithNeighbors_nolock(
                            lvlbox, neighbor_chunk_index,
                            neighbor_tile_index
                        );
                    } else if (!drag_at_ceiling && fabs(neighbor_tile->
                            segment[n_seg].floor_z[neighbor_corner] -
                            pre_drag_z) < 0.001) {
                        neighbor_tile->
                            segment[n_seg].floor_z[neighbor_corner] =
                                result_drag_z;
                        _spew3d_lvlbox_InvalidateTileWithNeighbors_nolock(
                            lvlbox, neighbor_chunk_index,
                            neighbor_tile_index
                        );
                    }
                    n_seg++;
                }
                _spew3d_lvlbox_EnforceValidFloors_nolock(
                    lvlbox, neighbor_chunk_index,
                    neighbor_tile_index,
                    (drag_at_ceiling ? n_seg + 1 : n_seg - 1)
                );
            }
        }
    }
    if (drag_at_ceiling) {
        tile->segment[segment_no].ceiling_z[corner] = result_drag_z;
    } else {
        tile->segment[segment_no].floor_z[corner] = result_drag_z;
    }
    _spew3d_lvlbox_EnforceValidFloors_nolock(
        lvlbox, chunk_index,
        tile_index,
        (drag_at_ceiling ? segment_no + 1 : segment_no - 1)
    );
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
    _spew3d_lvlbox_InvalidateTileWithNeighbors_nolock(
        lvlbox, chunk_index, tile_index
    );
    mutex_Release(_lvlbox_Internal(lvlbox)->m);
    return result;
}

S3DEXP int spew3d_lvlbox_edit_CycleTexturePaint(
        s3d_lvlbox *lvlbox, s3d_pos paint_pos,
        s3d_rotation paint_aim, uint8_t reverse_cycle
        ) {
    mutex_Lock(_lvlbox_Internal(lvlbox)->m);

    uint8_t topwallmodifier = 0;
    int32_t chunk_index, tile_index, segment_no;
    int32_t chunk_x, chunk_y, tile_x, tile_y;
    int corner_no, wall_no;
    int result = (
        _spew3d_lvlbox_InteractPosDirToTileCornerOrWall_nolock(
            lvlbox, paint_pos, paint_aim,
            &chunk_index, &tile_index, &chunk_x, &chunk_y,
            &tile_x, &tile_y, &segment_no, &corner_no, &wall_no,
            &topwallmodifier
        )
    );
    if (!result || segment_no < 0) {
        #if defined(DEBUG_SPEW3D_LVLBOX)
        printf("spew3d_lvlbox.c: debug: lvlbox %p "
            "S3DHID int spew3d_lvlbox_edit_PaintCycleTexture(): "
            "Not aiming at anything, with "
            "paint_pos x/y/z=%f/%f/%f "
            "paint_aim hori/verti/roll=%f/%f/%f "
            "input.\n",
            lvlbox, (double)paint_pos.x, (double)paint_pos.y,
            (double)paint_pos.z, (double)paint_aim.hori,
            (double)paint_aim.verti,
            (double)paint_aim.roll
        );
        #endif
        mutex_Release(_lvlbox_Internal(lvlbox)->m);
        return 1;
    }
    assert(segment_no >= 0 && (
        wall_no >= 0 || corner_no >= 0));;
    #if defined(DEBUG_SPEW3D_LVLBOX)
    printf("spew3d_lvlbox.c: debug: lvlbox %p "
        "S3DHID int spew3d_lvlbox_edit_PaintCycleTexture(): "
        "Aiming at item and doing cycle, with "
        "paint_pos x/y/z=%f/%f/%f "
        "paint_aim hori/verti/roll=%f/%f/%f "
        "input.\n",
        lvlbox, (double)paint_pos.x, (double)paint_pos.y,
        (double)paint_pos.z, (double)paint_aim.hori,
        (double)paint_aim.verti,
        (double)paint_aim.roll
    );
    #endif
    result = _spew3d_lvlbox_CycleTextureAtTileIdx_nolock(
        lvlbox, chunk_index, tile_index, segment_no,
        wall_no, (wall_no < 0 && paint_aim.verti < 0),
        (wall_no < 0 && paint_aim.verti >= 0),
        topwallmodifier, reverse_cycle
    );
    mutex_Release(_lvlbox_Internal(lvlbox)->m);
    return 1;
}

S3DEXP int spew3d_lvlbox_edit_PaintLastUsedFenceEx(
        s3d_lvlbox *lvlbox, s3d_pos paint_pos,
        s3d_rotation paint_aim
        ) {
    mutex_Lock(_lvlbox_Internal(lvlbox)->m);

    uint8_t topwallmodifier = 0;
    const char *paint_name = "grass01.png";
    int paint_vfsflags = 0;
    if (_lvlbox_Internal(lvlbox)->last_used_fence) {
        paint_name = _lvlbox_Internal(lvlbox)->last_used_fence;
        paint_vfsflags = _lvlbox_Internal(lvlbox)->
            last_used_fence_vfsflags;
    }

    int32_t chunk_index, tile_index, segment_no;
    int32_t chunk_x, chunk_y, tile_x, tile_y;
    int corner_no, wall_no;
    int result = _spew3d_lvlbox_InteractPosDirToTileCornerOrWall_nolock(
        lvlbox, paint_pos, paint_aim,
        &chunk_index, &tile_index, &chunk_x, &chunk_y,
        &tile_x, &tile_y, &segment_no, &corner_no, &wall_no,
        &topwallmodifier
    );
    if (!result) {
        mutex_Release(_lvlbox_Internal(lvlbox)->m);
        return 1;
    }

    s3d_lvlbox_tile *tile = &(
        lvlbox->chunk[chunk_index].tile[tile_index]
    );
    int opposite_wall = (wall_no >= 0 ?
        (wall_no + 2) % 4 : -1);
    int is_facing_floor = (
        wall_no < 0 && paint_aim.verti < 0
    );
    if (!tile->occupied || tile->segment_count < 0 ||
            segment_no < 0) {
        mutex_Release(_lvlbox_Internal(lvlbox)->m);
        return 1;
    }
    int32_t neighbor_chunk_index = -1;
    int32_t neighbor_tile_index = -1;
    s3d_lvlbox_tile *neighbor_tile = NULL;
    if (wall_no >= 0) {
        assert(opposite_wall >= 0);
        int shift_x = 0;
        int shift_y = 0;
        if (wall_no == 0) {
            shift_x = 1;
        } else if (wall_no == 1) {
            shift_y = 1;
        } else if (wall_no == 2) {
            shift_x = -1;
        } else {
            assert(wall_no == 3);
            shift_y = -1;
        }
        int result = _spew3d_lvlbox_GetNeighborTile_nolock(
            lvlbox, chunk_index, tile_index,
            shift_x, shift_y, &neighbor_chunk_index,
            &neighbor_tile_index
        );
        if (!result || !lvlbox->chunk[neighbor_chunk_index].
                tile[neighbor_tile_index].occupied) {
            neighbor_chunk_index = -1;
            neighbor_tile_index = -1;
        }
        if (neighbor_chunk_index >= 0 &&
                neighbor_tile_index >= 0) {
            neighbor_tile = &(
                lvlbox->chunk[neighbor_chunk_index].
                    tile[neighbor_tile_index]
            );
        }
    }
    double our_min_z = (
        tile->segment[segment_no].floor_z[0]
    );
    double our_max_z = (
        tile->segment[segment_no].ceiling_z[0]
    );
    double our_min_z_upper = (
        tile->segment[segment_no].floor_z[0]
    );
    double our_max_z_lower = (
        tile->segment[segment_no].ceiling_z[0]
    );
    int k = 1;
    while (k < 4) {
        our_min_z = fmin(
            tile->segment[segment_no].floor_z[k],
            our_min_z
        );
        our_min_z_upper = fmax(
            tile->segment[segment_no].floor_z[k],
            our_min_z_upper
        );
        our_max_z = fmax(
            tile->segment[segment_no].ceiling_z[k],
            our_max_z
        );
        our_max_z_lower = fmin(
            tile->segment[segment_no].ceiling_z[k],
            our_max_z_lower
        );
        k++;
    }
    if (neighbor_tile != NULL) {
        // Unset fences in all neighboring segments facing ours,
        // whenever they have shared openings to the side:
        int i = 0;
        while (i < neighbor_tile->segment_count) {
            double seg_min_z = (
                neighbor_tile->segment[i].floor_z[0]
            );
            double seg_max_z = (
                neighbor_tile->segment[i].ceiling_z[0]
            );
            int k = 1;
            while (k < 4) {
                seg_min_z = fmin(
                    neighbor_tile->segment[i].floor_z[k],
                    seg_min_z
                );
                seg_max_z = fmax(
                    neighbor_tile->segment[i].ceiling_z[k],
                    seg_max_z
                );
                k++;
            }
            if (seg_max_z < our_min_z || seg_min_z > our_max_z) {
                i++;
                continue;
            }
            if (neighbor_tile->segment[i].wall[opposite_wall].
                    fence.is_set) {
                free(neighbor_tile->segment[i].wall[opposite_wall].
                    fence.tex.name);
                neighbor_tile->segment[i].wall[opposite_wall].
                    fence.tex.name = NULL;
                neighbor_tile->segment[i].wall[opposite_wall].
                    fence.tex.id = 0;
                memset(&neighbor_tile->segment[i].
                    wall[opposite_wall].fence, 0,
                    sizeof(neighbor_tile->segment[i].
                    wall[opposite_wall].fence));
            }
            i++;
        }
    }

    if (wall_no < 0) {  // This is a horizontal fence:
        s3dnum_t add_fence_z = paint_pos.z + (
            paint_aim.verti > 0 ?
            (LVLBOX_TILE_SIZE * 0.25) :
            (-LVLBOX_TILE_SIZE)
        );
        int edit_candidate_idx = -1;
        s3dnum_t edit_candidate_z = 0;
        int cant_add_fence = 0;
        if (add_fence_z < our_min_z_upper ||
                add_fence_z > our_max_z_lower)
            cant_add_fence = 1;
        int k = 0;
        while (k < tile->segment[segment_no].
                hori_fence_count) {
            if (fabs(tile->segment[segment_no].
                    hori_fence_z[k] - add_fence_z) <
                    min_vertical_spacing) {
                cant_add_fence = 1;
            }
            if (fabs(tile->segment[segment_no].
                    hori_fence_z[k] - add_fence_z) <
                    edit_candidate_z ||
                    edit_candidate_idx < 0) {
                edit_candidate_idx = k;
                edit_candidate_z = tile->segment[segment_no].
                    hori_fence_z[k];
            }
            k++;
        }
        if (!cant_add_fence) {
            int hori_count = (
                tile->segment[segment_no].hori_fence_count
            );
            s3d_lvlbox_fenceinfo *new_hori_fence = realloc(
                tile->segment[segment_no].hori_fence,
                sizeof(*new_hori_fence) * (hori_count + 1)
            );
            if (!new_hori_fence) {
                mutex_Release(_lvlbox_Internal(lvlbox)->m);
                return 0;
            }
            tile->segment[segment_no].hori_fence = new_hori_fence;
            s3dnum_t *new_hori_fence_z = realloc(
                tile->segment[segment_no].hori_fence_z,
                sizeof(*new_hori_fence_z) * (hori_count + 1)
            );
            if (!new_hori_fence_z) {
                mutex_Release(_lvlbox_Internal(lvlbox)->m);
                return 0;
            }
            tile->segment[segment_no].hori_fence_z = new_hori_fence_z;
            memset(
                &tile->segment[segment_no].hori_fence[hori_count],
                0, sizeof(tile->segment[0].hori_fence[0])
            );
            tile->segment[segment_no].
                hori_fence[hori_count].tex.name =
                strdup(paint_name);
            if (!tile->segment[segment_no].
                    hori_fence[hori_count].tex.name) {
                mutex_Release(_lvlbox_Internal(lvlbox)->m);
                return 0;
            }
            tile->segment[segment_no].
                hori_fence[hori_count].tex.id =
                spew3d_texture_FromFile(
                    paint_name, paint_vfsflags
                );
            if (!tile->segment[segment_no].
                    hori_fence[hori_count].tex.id) {
                free(tile->segment[segment_no].
                    hori_fence[hori_count].tex.name);
                mutex_Release(_lvlbox_Internal(lvlbox)->m);
                return 0;
            }
            tile->segment[segment_no].
                hori_fence[hori_count].tex.vfs_flags =
                paint_vfsflags;
            tile->segment[segment_no].
                hori_fence[hori_count].is_set = 1;
            tile->segment[segment_no].
                hori_fence[hori_count].has_alpha = 1;

            tile->segment[segment_no].hori_fence_count++;
            mutex_Release(_lvlbox_Internal(lvlbox)->m);
            return 1;
        }
        if (edit_candidate_idx < 0) {
            mutex_Release(_lvlbox_Internal(lvlbox)->m);
            return 1;
        }
    }

    mutex_Release(_lvlbox_Internal(lvlbox)->m);
    return 1;
}

S3DHID int spew3d_lvlbox_edit_PaintLastUsedTextureEx(
        s3d_lvlbox *lvlbox, s3d_pos paint_pos,
        s3d_rotation paint_aim, int erase
        ) {
    mutex_Lock(_lvlbox_Internal(lvlbox)->m);

    uint8_t topwallmodifier = 0;
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
        &tile_x, &tile_y, &segment_no, &corner_no, &wall_no,
        &topwallmodifier
    );
    if (!result) {
        #if defined(DEBUG_SPEW3D_LVLBOX)
        printf("spew3d_lvlbox.c: debug: lvlbox %p "
            "spew3d_lvlbox_edit_PaintLastUsedTexture(): "
            "Not aiming at anything, with "
            "paint_pos x/y/z=%f/%f/%f "
            "paint_aim hori/verti/roll=%f/%f/%f "
            "input.\n",
            lvlbox, (double)paint_pos.x, (double)paint_pos.y,
            (double)paint_pos.z, (double)paint_aim.hori,
            (double)paint_aim.verti,
            (double)paint_aim.roll
        );
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
        "paint_pos x/y/z=%f/%f/%f "
        "paint_aim hori/verti/roll=%f/%f/%f "
        "input.\n",
        lvlbox,
        (int)chunk_index, (int)tile_index, (int)segment_no,
        (int)corner_no, (int)wall_no,
        (double)paint_pos.x, (double)paint_pos.y,
        (double)paint_pos.z, (double)paint_aim.hori,
        (double)paint_aim.verti,
        (double)paint_aim.roll
    );
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
        s3d_rotation paint_aim
        ) {
    return spew3d_lvlbox_edit_PaintLastUsedTextureEx(
        lvlbox, paint_pos, paint_aim, 0
    );
}

S3DEXP int spew3d_lvlbox_edit_EraseTexture(
        s3d_lvlbox *lvlbox, s3d_pos paint_pos,
        s3d_rotation paint_aim
        ) {
    return spew3d_lvlbox_edit_PaintLastUsedTextureEx(
        lvlbox, paint_pos, paint_aim, 1
    );
}

S3DHID int _spew3d_lvlbox_edit_AddNewLevelOfGround_nolock(
        s3d_lvlbox *lvlbox, s3d_pos paint_pos,
        s3d_rotation paint_aim
        ) {
    uint32_t chunk_index, tile_index;
    int32_t segment_no = -1;
    int result =  _spew3d_lvlbox_WorldPosToTilePos_nolock(
        lvlbox, paint_pos, 0, &chunk_index, &tile_index,
        NULL, NULL, &segment_no
    );

    if (!result) {
        // FIXME: We would want to add in a new tile here.
        return 1;
    }
    s3d_lvlbox_tile *tile = &(
        lvlbox->chunk[chunk_index].tile[tile_index]
    );
    if (!tile->occupied || tile->segment_count < 0) {
        // Just use the paint floor feature:
        if (_lvlbox_Internal(lvlbox)->last_used_tex == NULL)
            return 1;
        return _spew3d_lvlbox_SetFloorOrCeilOrWallTextureAt_nolock(
            lvlbox, paint_pos,
            _lvlbox_Internal(lvlbox)->last_used_tex,
            _lvlbox_Internal(lvlbox)->last_used_tex_vfsflags,
            0, -1, 1
        );
    }

    int insert_seg_no = -1;
    if (paint_pos.z > fmax(fmax(
                tile->segment[tile->segment_count - 1].floor_z[0],
                tile->segment[tile->segment_count - 1].floor_z[1]
            ), fmax(
                tile->segment[tile->segment_count - 1].floor_z[2],
                tile->segment[tile->segment_count - 1].floor_z[3]
            ))) {
        insert_seg_no = tile->segment_count;
    } else if (paint_pos.z < fmin(fmin(
                tile->segment[0].floor_z[0],
                tile->segment[0].floor_z[1]
            ), fmax(
                tile->segment[0].floor_z[2],
                tile->segment[0].floor_z[3]
            ))) {
        insert_seg_no = 0;
    } else if (segment_no >= 0) {
        // Always insert above segment we were located in:
        insert_seg_no = segment_no + 1;

        // Disallow if we're too close to the floor.
        if (paint_pos.z < fmax(fmax(
                    tile->segment[segment_no].floor_z[0],
                    tile->segment[segment_no].floor_z[1]
                ), fmax(
                    tile->segment[segment_no].floor_z[2],
                    tile->segment[segment_no].floor_z[3]
                ))) {
            return 1;
        }
    }

    // Figure out how much space we have above insert point:
    int have_floor_above = 0;
    double floor_above_z = 0;
    if (insert_seg_no >= 0 &&
            tile->segment_count > insert_seg_no) {
        // Abort if the floor above is too low.
        have_floor_above = 1;
        floor_above_z = tile->segment[insert_seg_no].floor_z[0];
        int i = 1;
        while (i < 4) {
            floor_above_z = fmin(
                tile->segment[insert_seg_no].floor_z[i],
                floor_above_z
            );
            i++;
        }
        if (paint_pos.z >= floor_above_z - min_vertical_spacing)
            return 1;
    }
    if (insert_seg_no < 0)
        return 1;

    // Add in a new segment at the given position:
    double new_floor_z = paint_pos.z - min_vertical_spacing * 2;
    double ceiling_below = paint_pos.z - min_vertical_spacing * 4;
    assert(tile->occupied && tile->segment_count > 0);
    if (insert_seg_no > 0) {
        int i = 0;
        while (i < 4) {
            if (tile->segment[insert_seg_no - 1].floor_z[i] >=
                    ceiling_below - min_vertical_spacing) {
                ceiling_below = fmax(ceiling_below,
                    tile->segment[insert_seg_no - 1].floor_z[i] +
                    min_vertical_spacing);
                new_floor_z = fmax(ceiling_below +
                    min_vertical_spacing * 2, new_floor_z);
            }
            i++;
        }
    }
    if (have_floor_above &&
            new_floor_z + min_vertical_spacing * 2 >=
            floor_above_z) {
        return 1;
    }
    s3d_lvlbox_vertsegment *new_seg = realloc(
        tile->segment, sizeof(*new_seg) *
        (tile->segment_count + 1)
    );
    if (!new_seg)
        return 0;
    tile->segment = new_seg;
    if (tile->segment_count > insert_seg_no) {
        // Move up the segments above us:
        memmove(&tile->segment[insert_seg_no + 1],
            &tile->segment[insert_seg_no],
            sizeof(*new_seg) * (
                tile->segment_count - insert_seg_no
            ));
    }
    memset(&tile->segment[insert_seg_no], 0,
        sizeof(tile->segment[insert_seg_no]));
    #if defined(DEBUG_SPEW3D_LVLBOX)
    printf("spew3d_lvlbox.c: "
        "debug: lvlbox %p "
        "_spew3d_lvlbox_edit_AddNewLevelOfGround_nolock(): "
        "Adding in new ground in segment slot %d "
        "(new segment count is %d) at "
        "chunk %d tile %d with new_floor_z=%f "
        "ceiling_below=%f.\n",
        lvlbox, (int)insert_seg_no, (int)tile->segment_count + 1,
        (int)chunk_index, (int)tile_index,
        (double)new_floor_z, (double)ceiling_below
    );
    #endif
    char *set_floor_tex_name =
        _lvlbox_Internal(lvlbox)->last_used_tex;
    int set_floor_tex_vfsflags =
        _lvlbox_Internal(lvlbox)->last_used_tex_vfsflags;
    char *set_ceiling_tex_name = NULL;
    int set_ceiling_tex_vfsflags = 0;
    assert(set_floor_tex_name != NULL);
    if (insert_seg_no > 0 &&
            tile->segment[insert_seg_no - 1].
            floor_tex.name != NULL) {
        set_floor_tex_name = (
            tile->segment[insert_seg_no - 1].
            floor_tex.name
        );
        set_floor_tex_vfsflags = (
            tile->segment[insert_seg_no - 1].
            floor_tex.vfs_flags
        );
    } else if (insert_seg_no < tile->segment_count &&
            tile->segment[insert_seg_no + 1].
            floor_tex.name != NULL) {
        set_floor_tex_name = (
            tile->segment[insert_seg_no + 1].
            floor_tex.name
        );
        set_floor_tex_vfsflags = (
            tile->segment[insert_seg_no + 1].
            floor_tex.vfs_flags
        );
    }
    if (insert_seg_no > 0 &&
            tile->segment[insert_seg_no - 1].
            ceiling_tex.name != NULL) {
        set_ceiling_tex_name = tile->segment[insert_seg_no - 1].
            ceiling_tex.name;
        set_ceiling_tex_vfsflags = (
            tile->segment[insert_seg_no - 1].
            ceiling_tex.vfs_flags
        );
    }
    char *new_last_used = strdup(set_floor_tex_name);
    char *new_assigned_floor_name = strdup(set_floor_tex_name);
    char *new_assigned_ceiling_name = (
        set_ceiling_tex_name != NULL ?
        strdup(set_ceiling_tex_name) : NULL
    );
    s3d_texture_t floor_tid = 0;
    if (new_assigned_floor_name != NULL)
        floor_tid = spew3d_texture_FromFile(
            new_assigned_floor_name,
            set_floor_tex_vfsflags
        );
    s3d_texture_t ceiling_tid = 0;
    if (new_assigned_ceiling_name != NULL)
        ceiling_tid = spew3d_texture_FromFile(
            new_assigned_ceiling_name,
            set_ceiling_tex_vfsflags
        );
    if (new_last_used == NULL ||
            new_assigned_floor_name == NULL ||
            floor_tid == 0 ||
            (set_ceiling_tex_name != NULL &&
            (new_assigned_ceiling_name == NULL ||
            ceiling_tid == 0))
            ) {
        free(new_last_used);
        free(new_assigned_floor_name);
        free(new_assigned_ceiling_name);
        return 0;
    }
    if (_lvlbox_Internal(lvlbox)->last_used_tex != NULL)
        free(_lvlbox_Internal(lvlbox)->last_used_tex);
    _lvlbox_Internal(lvlbox)->last_used_tex = new_last_used;
    _lvlbox_Internal(lvlbox)->last_used_tex_vfsflags =
        set_floor_tex_vfsflags;
    tile->segment[insert_seg_no].floor_tex.name =
        new_assigned_floor_name;
    tile->segment[insert_seg_no].floor_tex.vfs_flags =
        set_floor_tex_vfsflags;
    tile->segment[insert_seg_no].floor_tex.id = floor_tid;
    if (new_assigned_ceiling_name != NULL) {
        tile->segment[insert_seg_no].ceiling_tex.name =
            new_assigned_ceiling_name;
        tile->segment[insert_seg_no].ceiling_tex.vfs_flags =
            set_ceiling_tex_vfsflags;
        tile->segment[insert_seg_no].ceiling_tex.id = ceiling_tid;
    }

    int i = 0;
    while (i < 4) {
        assert(new_floor_z <
            floor_above_z - min_vertical_spacing);
        tile->segment[insert_seg_no].floor_z[i] =
            new_floor_z;
        tile->segment[insert_seg_no].ceiling_z[i] =
            fmin(
                new_floor_z + LVLBOX_TILE_SIZE * 2.0,
                (have_floor_above ? (floor_above_z -
                min_vertical_spacing) :
                (new_floor_z + LVLBOX_TILE_SIZE * 10))
            );
        if (insert_seg_no > 0) {
            tile->segment[insert_seg_no - 1].ceiling_z[i] = fmin(
                tile->segment[insert_seg_no - 1].ceiling_z[i],
                ceiling_below
            );
        }
        assert(
            tile->segment[insert_seg_no].ceiling_z[i] >
            tile->segment[insert_seg_no].floor_z[i]
        );
        i++;
    }
    tile->segment_count++;
    #ifndef NDEBUG
    int z = 0;
    while (z < tile->segment_count) {
        int i = 0;
        while (i < 4) {
            assert(
                z <= 0 ||
                tile->segment[z].floor_z[i] >
                tile->segment[z - 1].ceiling_z[i]
            );
            assert(
                tile->segment[z].floor_z[i] <
                tile->segment[z].ceiling_z[i]
            );
            assert(
                z >= tile->segment_count - 1 ||
                tile->segment[z].ceiling_z[i] <
                tile->segment[z + 1].floor_z[i]
            );
            i++;
        }
        z++;
    }
    #endif
    _spew3d_lvlbox_EnforceValidFloors_nolock(
        lvlbox, chunk_index, tile_index, insert_seg_no
    );
    _spew3d_lvlbox_InvalidateTileWithNeighbors_nolock(
        lvlbox, chunk_index, tile_index
    );
    return 1;
}

S3DEXP int spew3d_lvlbox_edit_AddNewLevelOfGround(
        s3d_lvlbox *lvlbox, s3d_pos paint_pos,
        s3d_rotation paint_aim
        ) {
    mutex_Lock(_lvlbox_Internal(lvlbox)->m);
    int result = _spew3d_lvlbox_edit_AddNewLevelOfGround_nolock(
        lvlbox, paint_pos, paint_aim
    );
    mutex_Release(_lvlbox_Internal(lvlbox)->m);
    return result;
}

S3DEXP int spew3d_lvlbox_edit_TryUseAsInputForEditing(
        s3d_window *editor_window,
        s3d_lvlbox *lvlbox, s3d_event *e, s3d_pos aim_pos,
        s3d_rotation aim_rot
        ) {
    uint32_t win_id = spew3d_window_GetID(editor_window);
    if (e->kind == S3DEV_MOUSE_MOVE) {
        if (_lvlbox_Internal(lvlbox)->_edit_dragging_floor) {
            // Mouse click dragging will adjust corner height.
            // (Shift key determines if neighboring tiles are
            // dragged along or not.)
            double drag_vert = -e->mouse.rel_y * 0.01;
            if (fabs(drag_vert) > 0.0001) {
                int r = spew3d_lvlbox_edit_DragFocusedTileCorner(
                    lvlbox, aim_pos, aim_rot, drag_vert,
                    !spew3d_keyboard_IsKeyPressed(
                        win_id, S3D_KEY_LEFTSHIFT
                    )
                );
            }
            return 1;
        }
        return 0;
    } else if (e->kind == S3DEV_MOUSEWHEEL_SCROLL) {
        // Mouse wheel cycles through available textures.
        if (!_lvlbox_Internal(lvlbox)->_edit_dragging_floor &&
                e->mousewheel.y > 0) {
            int r = spew3d_lvlbox_edit_CycleTexturePaint(
                lvlbox, aim_pos, aim_rot, 0
            );
            return 1;
        } else if (!_lvlbox_Internal(lvlbox)->_edit_dragging_floor &&
                e->mousewheel.y < 0) {
            int r = spew3d_lvlbox_edit_CycleTexturePaint(
                lvlbox, aim_pos, aim_rot, 0
            );
            return 1;
        }
        return 0;
    } else if (e->kind == S3DEV_KEY_DOWN &&
            e->key.key == S3D_KEY_T) {
        // Pressing T paints a texture.
        spew3d_lvlbox_edit_PaintLastUsedTexture(
            lvlbox, aim_pos, aim_rot
        );
        return 1;
    } else if (e->kind == S3DEV_KEY_DOWN &&
            e->key.key == S3D_KEY_G) {
        // Pressing G adds in a new level of ground.
        spew3d_lvlbox_edit_AddNewLevelOfGround(
            lvlbox, aim_pos, aim_rot
        );
        return 1;
    } else if (e->kind == S3DEV_MOUSE_BUTTON_DOWN &&
            e->mouse.button == S3DEV_MOUSE_BUTTON_PRIMARY
            ) {
        _lvlbox_Internal(lvlbox)->_edit_dragging_floor = 1;
        return 1;
    } else if (e->kind == S3DEV_MOUSE_BUTTON_UP &&
            e->mouse.button == S3DEV_MOUSE_BUTTON_PRIMARY
            ) {
        _lvlbox_Internal(lvlbox)->_edit_dragging_floor = 0;
        return 1;
    }
    return 0;
}

#endif  // SPEW3D_IMPLEMENTATION

