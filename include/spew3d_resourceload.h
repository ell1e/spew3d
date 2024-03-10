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

#ifndef SPEW3D_RESOURCELOAD_H_
#define SPEW3D_RESOURCELOAD_H_

typedef struct s3d_resourceload_job s3d_resourceload_job;

enum ResourceLoadType {
    RLTYPE_INVALID = 0,
    RLTYPE_IMAGE = 1,
    RLTYPE_LVLBOX = 2,
    RLTYPE_LVLBOX_STORE = 3,
    RLTYPE_LVLBOX_CYCLETEX = 4
};

typedef struct s3d_resourceload_result {
    int rltype;
    union {
        struct resource_image {
            void *pixels;
            uint64_t w, h;
        } resource_image;
        struct generic {
            void *callback_result;
        } generic;
    };
} s3d_resourceload_result;

S3DEXP s3d_resourceload_job *s3d_resourceload_NewJobWithCallback(
    const char *path, int rltype, int vfsflags,
    void *(*callback)(const char *path, int vfsflags,
        void *extradata),
    void *extradata
);

S3DEXP s3d_resourceload_job *s3d_resourceload_NewJob(
    const char *path, int rltype, int vfsflags
);

S3DEXP void s3d_resourceload_DestroyJob(
    s3d_resourceload_job *job
);

S3DEXP int s3d_resourceload_IsDone(s3d_resourceload_job *job);

S3DEXP int s3d_resourceload_ExtractResult(
    s3d_resourceload_job *job,
    s3d_resourceload_result *out_result,
    int *out_fserr
);

#endif  // SPEW3D_RESLOAD_H_

