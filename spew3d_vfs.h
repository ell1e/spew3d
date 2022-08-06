/* Spew3D is Copyright 2022 ell1e et al.

Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or
sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following
conditions:

The above copyright notice and this permission notice shall
be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY
KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS
OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/


#ifndef SPEW3D_VFS_H_
#define SPEW3D_VFS_H_

#include <stdint.h>


#define VFSFLAG_NO_REALDISK_ACCESS 1
#define VFSFLAG_NO_VIRTUALPAK_ACCESS 2

char *spew3d_vfs_NormalizePath(const char *path);

int spew3d_vfs_FileToBytes(
    const char *path,
    char **out_bytes,
    uint64_t *out_bytes_len
);

typedef struct SPEW3DVFS_FILE SPEW3DVFS_FILE;

SPEW3DVFS_FILE *spew3d_vfs_fopen(
    const char *path, const char *mode, int flags
);

SPEW3DVFS_FILE *spew3d_vfs_OwnThisFD(
    FILE *f, const char *reopenmode
);

void spew3d_vfs_DetachFD(SPEW3DVFS_FILE *f);

int spew3d_vfs_feof(SPEW3DVFS_FILE *f);

size_t spew3d_vfs_fread(
    char *buffer, int bytes, int numn, SPEW3DVFS_FILE *f
);   // sets errno = 0 on eof, errno = EIO on other error.

int64_t spew3d_vfs_ftell(SPEW3DVFS_FILE *f);

void spew3d_vfs_fclose(SPEW3DVFS_FILE *f);

int spew3d_vfs_fseek(SPEW3DVFS_FILE *f, uint64_t offset);

int spew3d_vfs_fseektoend(SPEW3DVFS_FILE *f);

int spew3d_vfs_fgetc(SPEW3DVFS_FILE *f);

int spew3d_vfs_peakc(SPEW3DVFS_FILE *f);

size_t spew3d_vfs_fwrite(
    const char *buffer, int bytes, int numn,
    SPEW3DVFS_FILE *f
);

SPEW3DVFS_FILE *spew3d_vfs_fdup(SPEW3DVFS_FILE *f);

int spew3d_vfs_flimitslice(
    SPEW3DVFS_FILE *f, uint64_t fileoffset, uint64_t maxlen
);

#endif  // SPEW3D_VFS_H_

