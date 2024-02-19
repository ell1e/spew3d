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

#include <string.h>

s3d_mutex *geometrydestroylist_m = NULL;
s3d_geometry **geometrydestroylist = NULL;
uint32_t geometrydestroylist_fill = 0;
uint32_t geometrydestroylist_alloc = 0;

S3DHID __attribute__((constructor))
        static void _geodestroylist_initmutex() {
    if (geometrydestroylist_m != NULL)
        return;
    geometrydestroylist_m = mutex_Create();
    if (!geometrydestroylist_m) {
        fprintf(stderr, "spew3d_geometry.c: error: "
            "Failed to allocate destruction list mutex.\n");
        _exit(1);
    }
}

S3DHID void _spew3d_geometry_ActuallyDestroy(
    s3d_geometry *geometry
);

S3DHID void _spew3d_geometry_ProcessDeletionsOnMainThread() {
    mutex_Lock(geometrydestroylist_m);
    uint32_t i = 0;
    while (i < geometrydestroylist_fill) {
        _spew3d_geometry_ActuallyDestroy(
            geometrydestroylist[i]
        );
        i++;
    }
    geometrydestroylist_fill = 0;
    mutex_Release(geometrydestroylist_m);
}

#endif  // SPEW3D_IMPLEMENTATION

