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

#ifndef SPEW3D_ARCHIVE_H_
#define SPEW3D_ARCHIVE_H_


#include <stdint.h>
#include <stdio.h>

typedef struct spew3darchive spew3darchive;
typedef enum spew3darchivetype {
    SPEW3DARCHIVE_TYPE_AUTODETECT = 0,
    SPEW3DARCHIVE_TYPE_ZIP = 1
} spew3darchivetype;

S3DEXP int64_t spew3d_archive_GetEntryCount(
    spew3darchive *a);

S3DEXP const char *spew3d_archive_GetEntryName(
    spew3darchive *a, uint64_t entry
);

S3DEXP int64_t spew3d_archive_GetEntrySize(
    spew3darchive *a, uint64_t entry
);

S3DEXP int spew3d_archive_GetEntryIsDir(
    spew3darchive *a, uint64_t entry
);

typedef enum spew3darchive_adderror {
    SPEW3DARCHIVE_ADDERROR_SUCCESS = 0,
    SPEW3DARCHIVE_ADDERROR_IOERROR = -1,
    SPEW3DARCHIVE_ADDERROR_OUTOFMEMORY = -2,
    SPEW3DARCHIVE_ADDERROR_INVALIDNAME = -3,
    SPEW3DARCHIVE_ADDERROR_DUPLICATENAME = -4
} spew3darchive_adderror;

S3DEXP int spew3d_archive_AddFileFromMem(
    spew3darchive *a, const char *filename,
    const char *bytes, uint64_t byteslen
);

S3DEXP int spew3d_archive_AddDir(
    spew3darchive *a, const char *dirname);

S3DEXP int spew3d_archive_GetEntryIndex(
    spew3darchive *a, const char *filename, int64_t *index,
    int *existsasfolder
);

S3DEXP int spew3d_archive_ReadFileByteSlice(
    spew3darchive *a, int64_t entry,
    uint64_t offset, char *buf, size_t readlen
);

S3DEXP void spew3darchive_Close(spew3darchive *a);

S3DEXP spew3darchive *spew3d_archive_FromFilePath(
    const char *path,
    int createifmissing, int vfsflags,
    spew3darchivetype type
);

S3DEXP spew3darchive *spew3d_archive_FromFilePathSlice(
    const char *path,
    uint64_t fileoffset, uint64_t maxlen,
    int createifmissing, int vfsflags, spew3darchivetype type
);

S3DEXP spew3darchive *spew3d_archive_FromFileHandleSlice(
    FILE *origf, uint64_t fileoffset, uint64_t maxlen,
    spew3darchivetype type, int fdiswritable
);

int spew3d_archive_IsCaseInsensitive(spew3darchive *a);

void spew3d_archive_SetCaseInsensitive(
    spew3darchive *a, int enabled
);

#endif  // SPEW3D_ARCHIVE_H_

