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

s3d_mutex *deletionqueue_m = NULL;
s3d_deletionqueueitem *deletionqueue = NULL;
uint32_t deletionqueue_fill = 0;
uint32_t deletionqueue_alloc = 0;

S3DHID __attribute__((constructor))
        static void _deletionqueue_initmutex() {
    if (deletionqueue_m != NULL)
        return;
    deletionqueue_m = mutex_Create();
    if (!deletionqueue_m) {
        fprintf(stderr, "spew3d_deletionqueue.c: error: "
            "Failed to allocate destruction list mutex.\n");
        _exit(1);
    }
}

S3DHID void _spew3d_window_ActuallyDestroy(
    s3d_window *geometry
);

S3DHID void _spew3d_geometry_ActuallyDestroy(
    s3d_geometry *geometry
);

S3DEXP void _spew3d_obj3d_DestroyActually(
    s3d_obj3d *obj
);

S3DHID static void _spew3d_lvlbox_ActuallyDestroy(
    s3d_lvlbox *lvlbox
);

S3DHID void _spew3d_deletionqueue_ProcessDeletionsOnMainThread() {
    mutex_Lock(deletionqueue_m);
    uint32_t i = 0;
    while (i < deletionqueue_fill) {
        if (deletionqueue[i].kind == DELETION_GEOM) {
            _spew3d_geometry_ActuallyDestroy(
                (s3d_geometry *)deletionqueue[i].item
            );
        } else if (deletionqueue[i].kind == DELETION_WINDOW) {
            _spew3d_window_ActuallyDestroy(
                (s3d_window *)deletionqueue[i].item
            );
        } else if (deletionqueue[i].kind == DELETION_OBJ3D) {
            _spew3d_obj3d_DestroyActually(
                (s3d_obj3d *)deletionqueue[i].item
            );
        } else if (deletionqueue[i].kind == DELETION_LVLBOX) {
            _spew3d_lvlbox_ActuallyDestroy(
                (s3d_lvlbox *)deletionqueue[i].item
            );
        }
        i++;
    }
    deletionqueue_fill = 0;
    mutex_Release(deletionqueue_m);
}

const char _delname_unknown[] = "(unknown)";
const char _delname_window[] = "s3d_window";
const char _delname_obj3d[] = "s3d_obj3d";
const char _delname_geom[] = "s3d_geometry";
S3DHID const char *_delkindname(int kind) {
    if (kind == DELETION_WINDOW) {
        return _delname_window;
    } else if (kind == DELETION_OBJ3D) {
        return _delname_obj3d;
    } else if (kind == DELETION_GEOM) {
        return _delname_geom;
    } else {
        return _delname_unknown;
    }
}

S3DEXP int spew3d_Deletion_Queue(int kind, void *item) {
    mutex_Lock(deletionqueue_m);
    if (deletionqueue_fill + 10 >
            deletionqueue_alloc) {
        uint32_t new_alloc = (
            deletionqueue_fill + 10
        ) * 2 + 16;
        s3d_deletionqueueitem *newdeletionqueue = realloc(
            deletionqueue,
            sizeof(*deletionqueue) * new_alloc
        );
        if (!newdeletionqueue) {
            fprintf(stderr, "spew3d_deletionqueue.c: error: "
                "Failed to allocate new destruction slots, "
                "MAY leak some memory.\n");
            if (deletionqueue_fill + 1 > deletionqueue_alloc) {
                fprintf(stderr, "spew3d_deletionqueue.c: "
                    "debug: Leak CONFIRMED.\n");
                mutex_Release(deletionqueue_m);
                return 0;
            } else {
                fprintf(stderr, "spew3d_deletionqueue.c: "
                    "debug: Leak for now AVOIDED.\n");
            }
        } else {
            deletionqueue = newdeletionqueue;
            deletionqueue_alloc = new_alloc;
        }
    }
    assert(deletionqueue_fill + 1 <= deletionqueue_alloc);
    memset(&deletionqueue[deletionqueue_fill], 0,
        sizeof(*deletionqueue));
    deletionqueue[deletionqueue_fill].kind = kind;
    deletionqueue[deletionqueue_fill].item = item;
    deletionqueue_fill++;
    #ifdef DEBUG_SPEW3D_DELETIONQUEUE
    fprintf(stderr, "spew3d_deletionqueue.c: debug: "
        "Listed item for deletion kind=%s ptr=%p.\n",
        _delkindname(kind), item);
    #endif
    mutex_Release(deletionqueue_m);
    return 1;
}

#endif  // SPEW3D_IMPLEMENTATION

