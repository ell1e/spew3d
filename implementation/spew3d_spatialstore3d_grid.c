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

typedef struct s3d_spatialstore3d_gridcell {
    s3d_obj3d *objlist;
    int objlist_alloc;
} s3d_spatialstore3d_gridcell;

typedef struct s3d_spatialstore3d_griddata {
    s3d_mutex *access;
    int cells_per_horizontal_axis,
        cells_per_vertical_axis;
    double cell_size_x, cell_size_y, cell_size_z;

    s3d_spatialstore3d_gridcell *contents;
} s3d_spatialstore3d_griddata;

S3DHID int s3d_spatialstore3d_GridAdd(
        s3d_spatialstore3d *store, s3d_obj3d *obj, 
        spew3d_pos pos, double extent_outer_radius
        ) {
    s3d_spatialstore3d_griddata *gdata = store->internal_data;
}

S3DHID int s3d_spatialstore3d_GridRemove(
        s3d_spatialstore3d *store) {
    s3d_spatialstore3d_griddata *gdata = store->internal_data;
}

S3DHID int s3d_spatialstore3d_GridFind(
        s3d_spatialstore3d *store, spew3d_pos searchpos,
        double searchrange, int expand_scan_by_collision_size,
        s3d_obj3d **out_list, int *out_count) {
    s3d_spatialstore3d_griddata *gdata = store->internal_data;
}

S3DHID int s3d_spatialstore3d_GridFindByCustomTypeNo(
        s3d_spatialstore3d *store,
        spew3d_pos searchpos,
        double searchrange, int expand_scan_by_collision_size,
        int *custom_type_no_list, int custom_type_no_list_len,
        s3d_obj3d **out_list, int *out_count) {
    s3d_spatialstore3d_griddata *gdata = store->internal_data;
}

S3DHID int s3d_spatialstore3d_GridFindEx(
        s3d_spatialstore3d *store, spew3d_pos searchpos,
        double searchrange, int expand_scan_by_collision_size,
        int *custom_type_no_list, int custom_type_no_list_len,
        s3d_obj3d **buffer_for_list,
        int buffer_alloc, s3d_obj3d **out_list,
        int *out_count, int *out_buffer_alloc) {
    s3d_spatialstore3d_griddata *gdata = store->internal_data;
}

S3DHID int s3d_spatialstore3d_GridFindClosest(
        s3d_spatialstore3d *store, spew3d_pos searchpos,
        double searchrange, int expand_scan_by_collision_size,
        s3d_obj3d *out_obj) {
    s3d_spatialstore3d_griddata *gdata = store->internal_data;
}

S3DEXP s3d_spatialstore3d *s3d_spatialstore3d_NewGridEx(
        double max_world_extent_per_axis, double max_collision_extent,
        int cells_per_horizontal_axis, int cells_per_vertical_axis
        ) {
    assert(cells_per_horizontal_axis >= 1 &&
        cells_per_vertical_axis >= 1);
    s3d_spatialstore3d_griddata *gdata = malloc(sizeof(*gdata));
    if (!gdata)
        return NULL;

    gdata->contents = malloc(
        cells_per_horizontal_axis * cells_per_horizontal_axis *
        cells_per_vertical_axis * sizeof(s3d_spatialstore3d_gridcell)
    );
    if (!gdata->contents) {
        free(gdata);
        return NULL;
    }

    s3d_spatialstore3d *store = malloc(sizeof(*store));
    if (!store) {
        free(store);
        free(gdata);
        return NULL;
    }
    store->internal_data = gdata;
    return store;
}

S3DEXP s3d_spatialstore3d *s3d_spatialstore3d_NewGrid(
        double max_coord_range, double max_regular_collision_size
        ) {
    
}

#endif  // SPEW3D_IMPLEMENTATION

