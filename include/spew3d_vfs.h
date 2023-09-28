/* Copyright (c) 2020-2023, ellie/@ell1e & Spew3D Team (see AUTHORS.md).

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

#ifndef SPEW3D_VFS_H_
#define SPEW3D_VFS_H_

#include <stdint.h>


#define VFSFLAG_NO_REALDISK_ACCESS 1
#define VFSFLAG_NO_VIRTUALPAK_ACCESS 2

// Mount an archive at the given path into the VFS space.
// Returns a positive numeric id for the mount on success,
// returns -1 on error.
int64_t spew3d_vfs_MountArchiveFromDisk(
    const char *path
);

char *spew3d_vfs_NormalizePath(const char *path);

int spew3d_vfs_FileToBytes(
    const char *path, int flags,
    int *out_fserr,
    char **out_bytes,
    uint64_t *out_bytes_len
);

int spew3d_vfs_Exists(
    const char *path, int vfsflags, int *result, int *fserr
);

typedef struct SPEW3DVFS_FILE SPEW3DVFS_FILE;

SPEW3DVFS_FILE *spew3d_vfs_fopen(
    const char *path, const char *mode, int flags
);

int spew3d_vfs_ferror(SPEW3DVFS_FILE *f);

SPEW3DVFS_FILE *spew3d_vfs_OwnThisFD(
    FILE *f, const char *reopenmode
);

void spew3d_vfs_DetachFD(SPEW3DVFS_FILE *f);

/// Check if the file position is at the end or not.
/// Returns 1 if position is at the end of the file,
/// otherwise 0.
int spew3d_vfs_feof(SPEW3DVFS_FILE *f);

size_t spew3d_vfs_fread(
    char *buffer, int bytes, int numn, SPEW3DVFS_FILE *f
);   // sets errno = 0 on eof, errno = EIO on other error.

int64_t spew3d_vfs_ftell(SPEW3DVFS_FILE *f);

void spew3d_vfs_fclose(SPEW3DVFS_FILE *f);

/// Seek to the given absolute offset.
/// Returns 0 on success, -1 on error.
int spew3d_vfs_fseek(SPEW3DVFS_FILE *f, uint64_t offset);

/// Seek to end of file. Returns 1 on success, 0 on error.
int spew3d_vfs_fseektoend(SPEW3DVFS_FILE *f);

/// Get next byte returned as value from 0 to 255,
/// or a negative value on error.
int spew3d_vfs_fgetc(SPEW3DVFS_FILE *f);

/// Peak at next byte but don't advance file position,
/// returns byte as value from 0 to 255, or a negative
/// value on error.
int spew3d_vfs_peakc(SPEW3DVFS_FILE *f);

size_t spew3d_vfs_fwrite(
    const char *buffer, int bytes, int numn,
    SPEW3DVFS_FILE *f
);

int spew3d_vfs_Size(
    const char *path, int vfsflags, uint64_t *result,
    int *fserr
);

SPEW3DVFS_FILE *spew3d_vfs_fdup(SPEW3DVFS_FILE *f);

int spew3d_vfs_flimitslice(
    SPEW3DVFS_FILE *f, uint64_t fileoffset, uint64_t maxlen
);

#endif  // SPEW3D_VFS_H_

