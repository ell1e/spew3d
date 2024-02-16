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

typedef struct s3d_gridobjentry {
    s3d_obj3d *obj;
    double extent_outer_radius;
    int is_static;
} s3d_gridobjentry;

typedef struct s3d_spatialstore3d_gridcell {
    s3d_gridobjentry *entrylist;
    uint32_t entrylist_fill, entrylist_alloc;
} s3d_spatialstore3d_gridcell;

typedef struct s3d_spatialstore3d_griddata {
    s3d_mutex *access;
    double max_coord_range;
    double max_regular_collision_size;
    uint32_t cells_per_horizontal_axis,
        cells_per_vertical_axis;
    double cell_size_x, cell_size_y, cell_size_z;
    s3d_pos center;

    s3d_gridobjentry *oversizedobjs;
    uint32_t oversizedobjs_fill, oversizedobjs_alloc;

    s3d_spatialstore3d_gridcell *contents;
} s3d_spatialstore3d_griddata;

S3DHID static void s3d_spatialstore3d_GridPosToCellCoords(
        s3d_spatialstore3d *store, s3d_pos searchpos,
        int32_t *out_x, int32_t *out_y, int32_t *out_z
        ) {
    s3d_spatialstore3d_griddata *gdata = store->internal_data;
    double grid_min_x = gdata->center.x - gdata->max_coord_range;
    double grid_max_x = gdata->center.x + gdata->max_coord_range;
    double grid_min_y = gdata->center.y - gdata->max_coord_range;
    double grid_max_y = gdata->center.y + gdata->max_coord_range;
    double grid_min_z = gdata->center.z - gdata->max_coord_range;
    double grid_max_z = gdata->center.z + gdata->max_coord_range;
    
    int32_t grid_offset_x = floor((double)
        (searchpos.x - grid_min_x) / gdata->cell_size_x
    );
    int32_t grid_offset_y = floor((double)
        (searchpos.y - grid_min_y) / gdata->cell_size_y
    );
    int32_t grid_offset_z = floor((double)
        (searchpos.z - grid_min_z) / gdata->cell_size_z
    );
    if (grid_offset_x < 0) grid_offset_x = 0;
    if (grid_offset_x >= gdata->cells_per_horizontal_axis)
        grid_offset_x = gdata->cells_per_horizontal_axis;
    if (grid_offset_y < 0) grid_offset_x = 0;
    if (grid_offset_y >= gdata->cells_per_horizontal_axis)
        grid_offset_y = gdata->cells_per_horizontal_axis;
    if (grid_offset_z < 0) grid_offset_z = 0;
    if (grid_offset_z >= gdata->cells_per_vertical_axis)
        grid_offset_z = gdata->cells_per_vertical_axis;
    *out_x = grid_offset_x;
    *out_y = grid_offset_y;
    *out_z = grid_offset_z;
}

S3DHID static int32_t s3d_spatialstore3d_GridPosToCellID(
        s3d_spatialstore3d *store, s3d_pos searchpos) {
    s3d_spatialstore3d_griddata *gdata = store->internal_data;
    int32_t x, y, z;
    s3d_spatialstore3d_GridPosToCellCoords(
        store, searchpos, &x, &y, &z
    );
    int32_t index = x + y * gdata->cells_per_horizontal_axis +
        z * gdata->cells_per_horizontal_axis *
        gdata->cells_per_horizontal_axis;
    return index;
}

S3DHID static int s3d_spatialstore3d_GridTestObjAgainstQuery(
        s3d_gridobjentry *entry,
        s3d_pos searchpos,
        double searchrange,
        int expand_search_by_collision_size,
        int32_t *custom_type_num_list,
        uint32_t custom_type_num_list_len) {
    s3d_obj3d *obj = entry->obj;
    s3d_pos opos = spew3d_obj3d_GetPos(obj);
    if (custom_type_num_list_len > 0) {
        int match = 0;
        uint32_t i = 0;
        while (i < custom_type_num_list_len) {
            if (spew3d_obj3d_HasCustomTypeNum(
                    obj, custom_type_num_list[i])) {
                match = 1;
                break;
            }
            i++;
        }
        if (!match)
            return 0;
    }
    double maxdist = searchrange;
    if (expand_search_by_collision_size) {
        maxdist += entry->extent_outer_radius;
    }
    if (spew3d_math3d_upperbounddist(&opos, &searchpos) > maxdist)
        return 0;
    if (spew3d_math3d_dist(&opos, &searchpos) > maxdist)
        return 0;
    return 1;
}

S3DHID int s3d_spatialstore3d_GridAdd(
        s3d_spatialstore3d *store,
        s3d_obj3d *obj,
        s3d_pos pos,
        double extent_outer_radius,
        int is_static
        ) {
    s3d_spatialstore3d_griddata *gdata = store->internal_data;

    if (extent_outer_radius > gdata->max_regular_collision_size) {
        if (gdata->oversizedobjs_fill + 1 >
                gdata->oversizedobjs_alloc) {
            uint32_t newalloc = gdata->oversizedobjs_fill + 1 + 32;
            newalloc *= 2;
            s3d_gridobjentry *newlist = realloc(
                gdata->oversizedobjs,
                sizeof(*gdata->oversizedobjs) *
                    newalloc);
            if (!newlist)
                return 0;
            gdata->oversizedobjs = newlist;
            gdata->oversizedobjs_alloc = newalloc;
        }
        s3d_gridobjentry *entry = &gdata->oversizedobjs[
            gdata->oversizedobjs_fill];
        gdata->oversizedobjs_fill++;
        memset(entry, 0, sizeof(*entry));
        entry->obj = obj;
        entry->extent_outer_radius = extent_outer_radius;
        entry->is_static = is_static;
        return 1;
    }

    int32_t index = s3d_spatialstore3d_GridPosToCellID(
        store, pos);
    s3d_spatialstore3d_gridcell *cell = &gdata->contents[index];
    if (cell->entrylist_fill + 1 > cell->entrylist_alloc) {
        uint32_t newalloc = cell->entrylist_fill + 1 + 32;
        newalloc *= 2;
        s3d_gridobjentry *newlist = realloc(
            cell->entrylist, sizeof(*cell->entrylist) *
                newalloc);
        if (!newlist)
            return 0;
        cell->entrylist = newlist;
        cell->entrylist_alloc = newalloc;
    }
    s3d_gridobjentry *entry = &cell->entrylist[cell->entrylist_fill];
    cell->entrylist_fill++;
    memset(entry, 0, sizeof(*entry));
    entry->obj = obj;
    entry->extent_outer_radius = extent_outer_radius;
    entry->is_static = is_static;
    return 1;
}

S3DHID int s3d_spatialstore3d_GridRemove(
        s3d_spatialstore3d *store, s3d_obj3d* obj) {
    s3d_spatialstore3d_griddata *gdata = store->internal_data;
    s3d_pos pos = spew3d_obj3d_GetPos(obj);

    uint32_t i = 0;
    while (i < gdata->oversizedobjs_fill) {
        if (gdata->oversizedobjs[i].obj == obj) {
            if (i + 1 < gdata->oversizedobjs_fill)
                memcpy(
                    &gdata->oversizedobjs[i],
                    &gdata->oversizedobjs[i + 1],
                    sizeof(*gdata->oversizedobjs) *
                        (gdata->oversizedobjs_fill - i - 1)
                );
            gdata->oversizedobjs_fill--;
            return 1;
        }
    }

    int32_t index = s3d_spatialstore3d_GridPosToCellID(
        store, pos);
    s3d_spatialstore3d_gridcell *cell = &gdata->contents[index];
    while (i < cell->entrylist_fill) {
        if (cell->entrylist[i].obj == obj) {
            if (i + 1 < gdata->oversizedobjs_fill)
                memcpy(
                    &cell->entrylist[i],
                    &cell->entrylist[i + 1],
                    sizeof(*cell->entrylist) *
                        (cell->entrylist_fill - i - 1)
                );
            cell->entrylist_fill--;
            return 1;
        }
    }
    return 0;
}

S3DHID int s3d_spatialstore3d_GridFindEx(
        s3d_spatialstore3d *store,
        s3d_pos searchpos,
        double searchrange,
        int expand_scan_by_collision_size,
        int32_t *custom_type_no_list,
        uint32_t custom_type_no_list_len,
        s3d_obj3d **buffer_for_list,
        int buffer_alloc,
        s3d_obj3d **out_list,
        uint32_t *out_count,
        uint32_t *out_buffer_alloc) {
    s3d_spatialstore3d_griddata *gdata = store->internal_data;
}

S3DHID int s3d_spatialstore3d_GridFindByCustomTypeNo(
        s3d_spatialstore3d *store,
        s3d_pos searchpos,
        double searchrange,
        int expand_scan_by_collision_size,
        int32_t *custom_type_no_list,
        int custom_type_no_list_len,
        s3d_obj3d **out_list,
        uint32_t *out_count) {
    return s3d_spatialstore3d_GridFindEx(
        store, searchpos, searchrange,
        expand_scan_by_collision_size,
        custom_type_no_list, custom_type_no_list_len,
        NULL, 0, out_list, out_count, NULL
    );
}

S3DHID int s3d_spatialstore3d_GridFind(
        s3d_spatialstore3d *store,
        s3d_pos searchpos,
        double searchrange,
        int expand_scan_by_collision_size,
        s3d_obj3d **out_list,
        uint32_t *out_count) {
    return s3d_spatialstore3d_GridFindByCustomTypeNo(
        store, searchpos, searchrange,
        expand_scan_by_collision_size,
        NULL, 0, out_list, out_count
    );
}

S3DHID int s3d_spatialstore3d_GridFindClosestByCustomTypeNo(
        s3d_spatialstore3d *store,
        s3d_pos searchpos,
        double searchrange,
        int expand_scan_by_collision_size,
        int32_t *custom_type_no_list,
        int custom_type_no_list_len,
        s3d_obj3d *out_obj) {
    s3d_spatialstore3d_griddata *gdata = store->internal_data;
}

S3DHID int s3d_spatialstore3d_GridFindClosest(
        s3d_spatialstore3d *store,
        s3d_pos searchpos,
        double searchrange,
        int expand_scan_by_collision_size,
        s3d_obj3d *out_obj) {
    return s3d_spatialstore3d_GridFindClosestByCustomTypeNo(
        store, searchpos, searchrange, expand_scan_by_collision_size,
        NULL, 0, out_obj);
}

S3DEXP s3d_spatialstore3d *s3d_spatialstore3d_NewGridEx(
        double max_coord_range,
        double max_regular_collision_size,
        int cells_per_horizontal_axis,
        int cells_per_vertical_axis,
        s3d_pos center
        ) {
    assert(cells_per_horizontal_axis >= 1 &&
        cells_per_vertical_axis >= 1);
    s3d_spatialstore3d_griddata *gdata = malloc(sizeof(*gdata));
    if (!gdata)
        return NULL;
    memset(gdata, 0, sizeof(*gdata));
    gdata->center = center;
    gdata->max_coord_range = max_coord_range;
    gdata->max_regular_collision_size = max_regular_collision_size;
    gdata->cells_per_horizontal_axis = cells_per_horizontal_axis;
    gdata->cells_per_vertical_axis = cells_per_vertical_axis;

    gdata->contents = malloc(
        cells_per_horizontal_axis * cells_per_horizontal_axis *
        cells_per_vertical_axis * sizeof(s3d_spatialstore3d_gridcell)
    );
    if (!gdata->contents) {
        free(gdata);
        return NULL;
    }
    memset(gdata->contents, 0, (
        cells_per_horizontal_axis * cells_per_horizontal_axis *
        cells_per_vertical_axis * sizeof(s3d_spatialstore3d_gridcell)
    ));

    s3d_spatialstore3d *store = malloc(sizeof(*store));
    if (!store) {
        free(gdata->contents);
        free(gdata);
        return NULL;
    }
    memset(store, 0, sizeof(*store));
    store->internal_data = gdata;
    
    store->Add = s3d_spatialstore3d_GridAdd;
    store->Remove = s3d_spatialstore3d_GridRemove;
    store->Find = s3d_spatialstore3d_GridFind;
    store->FindEx = s3d_spatialstore3d_GridFindEx;
    store->FindClosest = s3d_spatialstore3d_GridFindClosest;
    return store;
}

S3DEXP s3d_spatialstore3d *s3d_spatialstore3d_NewGrid(
        double max_coord_range,
        double max_regular_collision_size,
        s3d_pos center
        ) {
    if (max_regular_collision_size >= max_coord_range) {
        return NULL;
    }
    /*s3d_spatialstore3d *store = s3d_spatialstore3d_NewGridEx(
        double max_world_extent_per_axis, double max_collision_extent,
        int cells_per_horizontal_axis, int cells_per_vertical_axis
    );*/
}

#endif  // SPEW3D_IMPLEMENTATION

