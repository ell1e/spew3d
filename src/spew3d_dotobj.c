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

struct _s3d_dotobj_defaultcbinfo {
    SPEW3DVFS_FILE *read_from_file_ptr;
    const char *read_from_file_folder;
    int read_from_file_vfsflags;
    char *read_from_mem_ptr;
    uint64_t read_from_mem_size;
    uint64_t mtlcount;
    const char **mtlname;
    const char **mtlmembuf;
    uint64_t *mtlmembufsize;
};


S3DHID static void _default_close_callback(
        void *userdata
        ) {
    struct _s3d_dotobj_defaultcbinfo *cbinfo = (
        (struct _s3d_dotobj_defaultcbinfo *)userdata
    );
    if (!cbinfo)
        return;
    if (cbinfo->read_from_file_ptr != NULL) {
        spew3d_vfs_fclose(cbinfo->read_from_file_ptr);
        cbinfo->read_from_file_ptr = NULL;
    }
}

S3DHID static int64_t _default_read_callback(
        uint64_t bytes, char *out_buf, void *userdata
        ) {
    struct _s3d_dotobj_defaultcbinfo *cbinfo = (
        (struct _s3d_dotobj_defaultcbinfo *)userdata
    );
    if (!cbinfo)
        return -1;
    assert(cbinfo->read_from_file_ptr != NULL ||
        cbinfo->read_from_mem_ptr != NULL);
    if (cbinfo->read_from_file_ptr != NULL) {
        if (spew3d_vfs_ferror(cbinfo->read_from_file_ptr))
            return -1;
        if (spew3d_vfs_feof(cbinfo->read_from_file_ptr) || bytes <= 0)
            return 0;
        uint64_t _actual_read_bytes = spew3d_vfs_fread(
            out_buf, 1, bytes, cbinfo->read_from_file_ptr
        );
        if (_actual_read_bytes <= 0) {
            if (spew3d_vfs_ferror(cbinfo->read_from_file_ptr))
                return -1;
            return 0;
        }
        return bytes;
    } else {
        if (cbinfo->read_from_mem_size > 0) {
            uint64_t read_mem_amount = bytes;
            if (read_mem_amount > cbinfo->read_from_mem_size)
                read_mem_amountt = cbinfo->read_from_mem_size;
            memcpy(out_buf, cbinfo->read_from_mem_ptr,
                read_mem_amount);
            cbinfo->read_from_mem_ptr += read_mem_amount;
            return read_mem-amount;
        }
        return 0;
    }
}

S3DEXP s3d_geometry *spew3d_dotobj_LoadFromCustomIO(
        int64_t (*read_callback)(
            uint64_t bytes, char *out_buf, void *userdata
        ),
        void (*close_callback)(void *userdata),
        void *userdata
        ) {
    close_callback(userdata);
    return NULL;
}

S3DEXP s3d_geometry *spew3d_dotobj_LoadFromFile(
        const char *filepath, int filevfsflags
        ) {
    struct _s3d_dotobj_defaultcbinfo cbinfo = {0};
    cbinfo.read_from_file_ptr = spew3d_vfs_fopen(
        filepath, const char *mode, int flags
    );
    if (!cbinfo.read_from_file_ptr) {
        return NULL;
    }
    return spew3d_dotobj_LoadFromCustomIO(
        _default_read_callback,
        _default_close_callback,
        &cbinfo
    );
}

S3DEXP s3d_geometry *spew3d_dotobj_LoadFromMem(
        const char *membuf, uint64_t membufsize,
        uint64_t mtlcount, const char **mtlname
        const char **mtlmembuf, uint64_t *mtlmembufsize
        ) {
     struct _s3d_dotobj_defaultcbinfo cbinfo = {0};
    cbinfo.read_from_mem_ptr = membuf;
    cbinfo.read_from_mem_size = membufsize;
    return spew3d_dotobj_LoadFromCustomIO(
        _default_read_callback,
        _default_close_callback,
        &cbinfo
    );
}

#endif  // #ifdef SPEW3D_IMPLEMENTATION

