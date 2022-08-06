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

#ifndef SPEW3D_ARCHIVE_H_
#define SPEW3D_ARCHIVE_H_


#include <stdint.h>
#include <stdio.h>

typedef struct spew3darchive spew3darchive;
typedef enum spew3darchivetype {
    SPEW3DARCHIVE_TYPE_AUTODETECT = 0,
    SPEW3DARCHIVE_TYPE_ZIP = 1
} spew3darchivetype;

int64_t spew3d_archive_GetEntryCount(
    spew3darchive *a);

const char *spew3d_archive_GetEntryName(
    spew3darchive *a, uint64_t entry);

int64_t spew3d_archive_GetEntrySize(
    spew3darchive *a, uint64_t entry);

int spew3d_archive_GetEntryIsDir(
    spew3darchive *a, uint64_t entry);

typedef enum spew3darchive_adderror {
    SPEW3DARCHIVE_ADDERROR_SUCCESS = 0,
    SPEW3DARCHIVE_ADDERROR_IOERROR = -1,
    SPEW3DARCHIVE_ADDERROR_OUTOFMEMORY = -2,
    SPEW3DARCHIVE_ADDERROR_INVALIDNAME = -3,
    SPEW3DARCHIVE_ADDERROR_DUPLICATENAME = -4
} spew3darchive_adderror;

int spew3d_archive_AddFileFromMem(
    spew3darchive *a, const char *filename,
    const char *bytes, uint64_t byteslen
);

int spew3d_archive_AddDir(
    spew3darchive *a, const char *dirname);

int spew3d_archive_GetEntryIndex(
    spew3darchive *a, const char *filename, int64_t *index,
    int *existsasfolder
);

int spew3d_archive_ReadFileByteSlice(
    spew3darchive *a, int64_t entry,
    uint64_t offset, char *buf, size_t readlen
);

void spew3darchive_Close(spew3darchive *a);

spew3darchive *spew3d_archive_FromFilePath(
    const char *path,
    int createifmissing, int vfsflags,
    spew3darchivetype type
);

spew3darchive *spew3d_archive_FromFilePathSlice(
    const char *path,
    uint64_t fileoffset, uint64_t maxlen,
    int createifmissing, int vfsflags, spew3darchivetype type
);

spew3darchive *spew3d_archive_FromFileHandleSlice(
    FILE *origf, uint64_t fileoffset, uint64_t maxlen,
    spew3darchivetype type, int fdiswritable
);

#endif  // SPEW3D_ARCHIVE_H_

