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

#include <stdint.h>

typedef struct s3d_lvlbox_internal {
    int wasdeleted;
    s3d_mutex *m;
} s3d_lvlbox_internal;

S3DHID static s3d_lvlbox_internal *_lvlbox_Internal(
        s3d_lvlbox *lvlbox
        ) {
    return (s3d_lvlbox_internal *)lvlbox->_internal;
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
        int32_t chunk_idx,
        int32_t tile_idx, int segment_no
        ) {
    assert(lvlbox != NULL);
    s3d_lvlbox_tile *tile = (
        &lvlbox->chunk[chunk_idx].tile[tile_idx]
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
        int32_t chunk_idx,
        int32_t tile_idx, int segment_no
        ) {
    mutex_Lock(_lvlbox_Internal(lvlbox)->m);
    double result = _spew3d_lvlbox_TileFloorHeightAtInSegment_nolock(
        lvlbox, local_pos, chunk_idx, tile_idx, segment_no
    );
    mutex_Release(_lvlbox_Internal(lvlbox)->m);
    return result;
}

S3DHID int _spew3d_lvlbox_WorldPosToTilePos_nolock(
        s3d_lvlbox *lvlbox, s3d_pos pos, int ignore_lvlbox_offset,
        int32_t *out_chunk_idx, int32_t *out_tile_idx,
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
    int32_t chunk_idx = _spew3d_lvlbox_WorldPosToChunkIndex_nolock(
        lvlbox, pos, 1
    );
    if (chunk_idx < 0)
        return 0;
    assert(chunk_idx < lvlbox->chunk_count);

    int32_t chunk_x = (chunk_idx % lvlbox->chunk_extent_x);
    int32_t chunk_y = (chunk_idx - chunk_x) / lvlbox->chunk_extent_x;
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
    int32_t tile_idx = (
        tile_y * (int32_t)LVLBOX_CHUNK_SIZE + tile_x
    );
    assert(tile_idx >= 0 &&
        tile_idx < (int32_t)(LVLBOX_CHUNK_SIZE * LVLBOX_CHUNK_SIZE));
    if (out_chunk_idx != NULL)
        *out_chunk_idx = chunk_idx;
    if (out_tile_idx != NULL)
        *out_tile_idx = tile_idx;
    if (out_tile_pos_offset != NULL || out_segment_no != NULL) {
        s3d_lvlbox_tile *tile = (
            &lvlbox->chunk[chunk_idx].tile[tile_idx]
        );
        s3d_pos offset = {0};
        offset.x = pos.x - (s3dnum_t)tile_x *
            (s3dnum_t)LVLBOX_TILE_SIZE;
        offset.y = pos.y - (s3dnum_t)tile_y *
            (s3dnum_t)LVLBOX_TILE_SIZE;
        int32_t segment_no = -1;
        if (tile->occupied) {
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
                if ((offset.z > floor_min_z || i == 0) &&
                        (offset.z < floor_min_z ||
                        i >= tile->segment_count - 1)) {
                    break;
                }
                i++;
            }
            assert(i < tile->segment_count);
            segment_no = i;
        }
        if (out_tile_pos_offset != NULL) {
            if (segment_no >= 0) {
                offset.z -= (
                    _spew3d_lvlbox_TileFloorHeightAtInSegment_nolock(
                        lvlbox, offset, chunk_idx, tile_idx, segment_no
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
        if (lvlbox->chunk[chunk_idx].tile[tile_idx].occupied) {
            double min_z = INT32_MAX;
            int k = 0;
            while (k < 4) {
                min_z = fmin(min_z,
                    lvlbox->chunk[chunk_idx].
                    tile[tile_idx].segment[0].floor_z[k]);
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
        int32_t *out_chunk_idx, int32_t *out_tile_idx,
        s3d_pos *out_tile_lower_bound,
        s3d_pos *out_tile_pos_offset,
        int32_t *out_segment_no
        ) {
    mutex_Lock(_lvlbox_Internal(lvlbox)->m);
    int32_t idx = _spew3d_lvlbox_WorldPosToTilePos_nolock(
        lvlbox, pos, ignore_lvlbox_offset,
        out_chunk_idx, out_tile_idx,
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
            memcpy(&new_chunk[chunk_offset_new],
                &lvlbox->chunk[chunk_offset_old],
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
            mutex_Release(_lvlbox_Internal(lvlbox)->m);
            return 0;
        }
    }
    if (expand_plus_chunkx > 0 || expand_minus_chunkx > 0) {
        int result = _spew3d_lvlbox_ResizeChunksY_nolock(
            lvlbox, lvlbox->chunk_extent_x +
            expand_plus_chunkx + expand_minus_chunkx
        );
        if (!result) {
            mutex_Release(_lvlbox_Internal(lvlbox)->m);
            return 0;
        }
    }

    mutex_Release(_lvlbox_Internal(lvlbox)->m);
    return 1;
}

struct lvlbox_load_settings {
    int create_file_if_missing;
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

S3DEXP s3d_resourceload_job *spew3d_lvlbox_FromMapFile(
        const char *map_file_path, int map_file_vfs_flags,
        int create_file_if_missing
        ) {
    struct lvlbox_load_settings *settings = malloc(sizeof(*settings));
    if (!settings)
        return NULL;
    memset(settings, 0, sizeof(*settings));
    settings->create_file_if_missing = 1;
    s3d_resourceload_job *job = s3d_resourceload_NewJobWithCallback(
        map_file_path, RLTYPE_LVLBOX, map_file_vfs_flags,
        _spew3d_lvlbox_DoMapLoad, settings
    );
    return job;
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

S3DEXP char *spew3d_lvlbox_ToString(
        s3d_lvlbox *lvlbox, uint32_t *out_slen
        ) {
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
        if (!spew3d_lvlbox_SetFloorTextureAt(
                lvlbox, pos, default_tex, default_tex_vfs_flags
                )) {
            _spew3d_lvlbox_ActuallyDestroy(lvlbox);
            return NULL;
        }
    }
    return lvlbox;
}

S3DEXP int spew3d_lvlbox_SetFloorTextureAt(
        s3d_lvlbox *lvlbox, s3d_pos pos,
        const char *texname, int vfsflags
        ) {
    if (!spew3d_lvlbox_ExpandToPosition(lvlbox, pos)) {
        return 0;
    }
    int32_t chunk_idx, tile_idx;
    int32_t segment_no = -1;
    s3d_pos tile_lower_bound;
    s3d_pos pos_offset;
    mutex_Lock(_lvlbox_Internal(lvlbox)->m);
    int result = _spew3d_lvlbox_WorldPosToTilePos_nolock(
        lvlbox, pos, 0, &chunk_idx, &tile_idx,
        &tile_lower_bound, &pos_offset, &segment_no
    );
    if (!result || tile_idx < 0) {
        mutex_Release(_lvlbox_Internal(lvlbox)->m);
        return 0;
    }
    s3d_lvlbox_tile *tile = &(
        lvlbox->chunk[chunk_idx].tile[tile_idx]
    );
    char *set_tex_name = strdup(texname);
    if (!set_tex_name) {
        mutex_Release(_lvlbox_Internal(lvlbox)->m);
        return 0;
    }
    s3d_texture_t tid = spew3d_texture_FromFile(
        set_tex_name, vfsflags
    );
    if (tid == 0) {
        free(set_tex_name);
        mutex_Release(_lvlbox_Internal(lvlbox)->m);
        return 0;
    }
    if (!tile->occupied) {
        assert(segment_no < 0);
        assert(tile->segment == NULL);
        tile->segment = malloc(sizeof(*tile->segment));
        if (!tile->segment) {
            free(set_tex_name);
            mutex_Release(_lvlbox_Internal(lvlbox)->m);
            return 0;
        }
        tile->occupied = 1;
        memset(tile->segment, 0, sizeof(*tile->segment) * 1);
        tile->segment_count = 1;
        int i = 0;
        while (i < 4) {
            tile->segment[0].floor_z[i] = pos.z - (
                (s3dnum_t)LVLBOX_DEFAULT_TILE_HEIGHT / 2.0
            );
            tile->segment[0].ceiling_z[i] = (
                tile->segment[0].floor_z[i] + (
                    (s3dnum_t)LVLBOX_DEFAULT_TILE_HEIGHT
                )
            );
            i += 4;
        }
        segment_no = 0;
    }
    assert(segment_no >= 0 && segment_no < tile->segment_count);
    tile->segment[segment_no].floor_tex.name = set_tex_name;
    tile->segment[segment_no].floor_tex.vfs_flags = vfsflags;
    tile->segment[segment_no].floor_tex.id = tid;
    mutex_Release(_lvlbox_Internal(lvlbox)->m);
    return 1;
}

#endif  // SPEW3D_IMPLEMENTATION

