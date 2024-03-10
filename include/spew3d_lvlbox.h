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

#ifndef SPEW3D_LVLBOX_H_
#define SPEW3D_LVLBOX_H_

#define LVLBOX_CHUNK_SIZE 6
#define LVLBOX_TILE_SIZE 1.0
#define LVLBOX_DEFAULT_TILE_HEIGHT 2.0

typedef struct s3d_resourceload_job s3d_resourceload_job;
typedef uint64_t s3d_texture_t;

enum Spew3dLvlboxTexwrapMode {
    S3D_LVLBOX_TEXWRAP_MODE_DEFAULT = 0,
    S3D_LVLBOX_TEXWRAP_MODE_AUTO = 1,
    S3D_LVLBOX_TEXWRAP_MODE_CUBIC = 3,
    S3D_LVLBOX_TEXWRAP_MODE_STRETCHED = 4
};

typedef struct s3d_lvlbox_texinfo {
    char *name;
    int vfs_flags;
    s3d_texture_t id;
    s3d_material_t material;
    int wrapmode;
    s3dnum_t scale_x, scale_y;
    s3dnum_t scroll_speed_x, scroll_speed_y;
} s3d_lvlbox_texinfo;

typedef struct s3d_lvlbox_fenceinfo {
    int fence_set, fence_hasalpha, fence_passable;
    double fence_height;
    s3d_lvlbox_texinfo fence_tex;
} s3d_lvlbox_fenceinfo;

typedef struct s3d_lvlbox_wallinfo {
    s3d_lvlbox_texinfo tex;

    s3d_lvlbox_fenceinfo fence;
} s3d_lvlbox_wallinfo;

typedef struct s3d_lvlbox_tilepolygon {
    s3d_pos vertex[3];
    s3d_point texcoord[3];
    s3d_pos polynormal;
    s3d_pos normal[3];
    s3d_color light_emit[3];
    s3d_material_t material;
    s3d_texture_t texture;
} s3d_lvlbox_tilepolygon;

typedef struct s3d_lvlbox_tilecache {
    uint8_t is_up_to_date, flat_normals_set;

    uint16_t cached_floor_polycount;
    uint16_t cached_floor_maxpolycount;
    s3d_lvlbox_tilepolygon *cached_floor;
    s3d_pos floor_flat_corner_normals[4];
    s3d_pos floor_smooth_corner_normals[4];
    int floor_split_from_front_left;

    uint16_t cached_ceiling_polycount;
    uint16_t cached_ceiling_maxpolycount;
    s3d_lvlbox_tilepolygon *cached_ceiling;
    s3d_pos ceiling_flat_corner_normals[4];
    s3d_pos ceiling_smooth_corner_normals[4];
    int ceiling_split_from_front_left;

    uint16_t cached_wall_polycount;
    uint16_t cached_wall_maxpolycount;
    s3d_lvlbox_tilepolygon *cached_wall;
} s3d_lvlbox_tilecache;

typedef struct s3d_lvlbox_vertsegment {
    s3d_lvlbox_texinfo floor_tex;
    s3dnum_t floor_z[4];

    s3d_lvlbox_wallinfo wall[4];
    s3d_lvlbox_wallinfo topwall[4];

    s3d_lvlbox_texinfo ceiling_tex;
    s3dnum_t ceiling_z[4];

    s3d_lvlbox_tilecache cache;
} s3d_lvlbox_vertsegment;

typedef struct s3d_lvlbox_tile {
    int occupied;

    s3d_lvlbox_vertsegment *segment;
    int segment_count;
} s3d_lvlbox_tile;

typedef struct s3d_lvlbox_chunk {
    s3d_lvlbox_tile tile[LVLBOX_CHUNK_SIZE *
        LVLBOX_CHUNK_SIZE];
} s3d_lvlbox_chunk;

typedef struct s3d_lvlbox {
    uint64_t gid;
    s3d_pos offset;

    s3d_lvlbox_chunk *chunk;
    uint32_t chunk_count, chunk_extent_x;

    void *_internal;
} s3d_lvlbox;

S3DEXP s3d_lvlbox *spew3d_lvlbox_New(
    const char *default_tex, int default_tex_vfs_flags
);

S3DEXP void spew3d_lvlbox_Destroy(
    s3d_lvlbox *lvlbox
);

S3DEXP int32_t spew3d_lvlbox_WorldPosToChunkIndex(
    s3d_lvlbox *lvlbox, s3d_pos pos, int ignore_lvlbox_offset
);

S3DEXP int spew3d_lvlbox_GetTileClosestCornerNearWorldPos(
    s3d_lvlbox *lvlbox, uint32_t chunk_index,
    uint32_t tile_index, int segment_no,
    s3d_pos pos, int ignore_z, int at_ceiling,
    s3d_pos *out_corner_pos
);
S3DEXP int spew3d_lvlbox_CornerPosToWorldPos(
    s3d_lvlbox *lvlbox, uint32_t chunk_index,
    uint32_t tile_index, int32_t segment_no,
    int corner_no, int at_ceiling, s3d_pos *out_pos
);

S3DEXP int spew3d_lvlbox_WorldPosToTilePos(
    s3d_lvlbox *lvlbox, s3d_pos pos, int ignore_lvlbox_offset,
    uint32_t *out_chunk_index, uint32_t *out_tile_index,
    s3d_pos *out_tile_lower_bound,
    s3d_pos *out_tile_pos_offset,
    int32_t *out_segment_no
);

S3DEXP int spew3d_lvlbox_HoriAngleToCornerIndex(
    s3dnum_t angle
);

S3DEXP double spew3d_lvlbox_TileFloorHeightAtInSegment(
    s3d_lvlbox *lvlbox, s3d_pos local_pos,
    int32_t chunk_idx,
    int32_t tile_idx, int segment_no
);

S3DEXP int spew3d_lvlbox_ExpandToPosition(
    s3d_lvlbox *box, s3d_pos pos
);

S3DEXP int spew3d_lvlbox_SetFloorTextureAt(
    s3d_lvlbox *lvlbox, s3d_pos pos,
    const char *texname, int vfsflags
);

S3DEXP int spew3d_lvlbox_SetCeilingTextureAt(
    s3d_lvlbox *lvlbox, s3d_pos pos,
    const char *texname, int vfsflags
);

S3DEXP s3d_resourceload_job *spew3d_lvlbox_FromMapFile(
    const char *map_file_path, int map_file_vfs_flags
);

S3DEXP s3d_resourceload_job *spew3d_lvlbox_FromMapFileOrNew(
    const char *map_file_path, int map_file_vfs_flags,
    int new_if_missing,
    const char *new_default_tex, int new_default_tex_vfs_flags
);

S3DEXP s3d_lvlbox *spew3d_lvlbox_FromMapFileFinalize(
    s3d_resourceload_job *job
);

S3DEXP s3d_resourceload_job *spew3d_lvlbox_ToMapFile(
    s3d_lvlbox *lvlbox,
    const char *map_file_path, int map_file_vfs_flags
);

S3DEXP char *spew3d_lvlbox_ToString(
    s3d_lvlbox *lvlbox, uint32_t *out_slen
);

S3DEXP s3d_lvlbox *spew3d_lvlbox_FromString(
    const char *s, uint32_t slen
);

S3DEXP int spew3d_lvlbox_Transform(
    s3d_lvlbox *lvlbox,
    s3d_pos *model_pos,
    s3d_rotation *model_rotation,
    s3d_transform3d_cam_info *cam_info,
    s3d_geometryrenderlightinfo *render_light_info,
    s3d_renderpolygon **render_queue,
    uint32_t *render_fill, uint32_t *render_alloc
);

S3DEXP int spew3d_lvlbox_edit_PaintLastUsedTexture(
    s3d_lvlbox *lvlbox, s3d_pos paint_pos,
    s3d_rotation paint_aim
);

S3DEXP int spew3d_lvlbox_edit_EraseTexture(
    s3d_lvlbox *lvlbox, s3d_pos paint_pos,
    s3d_rotation paint_aim
);

S3DEXP int spew3d_lvlbox_edit_AddNewLevelOfGround(
    s3d_lvlbox *lvlbox, s3d_pos paint_pos,
    s3d_rotation paint_aim
);

S3DEXP int spew3d_lvlbox_edit_DragFocusedTileCorner(
    s3d_lvlbox *lvlbox, s3d_pos drag_pos,
    s3d_rotation drag_aim, double drag_z,
    int dragconnected
);

S3DEXP int spew3d_lvlbox_InteractPosDirToTileCorner(
    s3d_lvlbox *lvlbox, s3d_pos interact_pos,
    s3d_rotation interact_rot,
    int32_t *chunk_index, int32_t *tile_index,
    int32_t *chunk_x, int32_t *chunk_y,
    int32_t *tile_x, int32_t *tile_y,
    int32_t *segment_no, int *corner_no
);

S3DEXP int spew3d_lvlbox_InteractPosDirToTileCornerOrWall(
    s3d_lvlbox *lvlbox, s3d_pos interact_pos,
    s3d_rotation interact_rot,
    int32_t *out_chunk_index, int32_t *out_tile_index,
    int32_t *out_chunk_x, int32_t *out_chunk_y,
    int32_t *out_tile_x, int32_t *out_tile_y,
    int32_t *out_segment_no, int *out_corner_no,
    int *out_wall_no
);

S3DEXP s3d_lvlbox *spew3d_lvlbox_GetByID(uint64_t id);

S3DEXP uint64_t spew3d_lvlbox_GetID(s3d_lvlbox *lvlbox);

#endif  // SPEW3D_LVLBOX_H_

