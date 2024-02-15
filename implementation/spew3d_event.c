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

typedef struct s3dequeue {
    int fill, alloc;
    s3devent *array;
    s3d_mutex *accesslock;
} s3dequeue;

S3DEXP s3dequeue *s3devent_q_Create() {
    s3dequeue *eq = malloc(sizeof(*eq));
    if (!eq)
        return NULL;
    memset(eq, sizeof(*eq), 0);
    eq->alloc = 1024;
    eq->array = malloc(sizeof(*eq->array) * eq->alloc);
    if (!eq->array) {
        free(eq);
        return NULL;
    }
    memset(eq->array, sizeof(*eq->array) * eq->alloc, 0);
    eq->accesslock = mutex_Create();
    if (!eq->accesslock) {
        free(eq->array);
        free(eq);
        return NULL;
    }
    return eq;
}

s3dequeue *_main_event_queue = NULL;
s3dequeue *_internal_event_queue = NULL;

S3DHID __attribute__((constructor)) static void _make_main_queue() {
    if (_main_event_queue != NULL &&
            _internal_event_queue != NULL)
        return;
    if (!_main_event_queue)
        _main_event_queue = s3devent_q_Create();
    if (!_internal_event_queue)
        _internal_event_queue = s3devent_q_Create();
}

S3DEXP s3dequeue *s3devent_GetMainQueue() {
    _make_main_queue();
    return _main_event_queue;
}

S3DHID s3dequeue *_s3devent_GetInternalQueue() {
    _make_main_queue();
    return _internal_event_queue;
}

S3DHID void _s3devent_q_InsertForce(s3dequeue *eq, const s3devent *ev) {
    while (1) {
        if (s3devent_q_Insert(eq, ev))
            return;

        spew3d_time_Sleep(20);
    }
}

S3DEXP int s3devent_q_Insert(s3dequeue *eq, const s3devent *ev) {
    mutex_Lock(eq->accesslock);
    if (eq->fill + 1 > eq->alloc) {
        int newalloc = eq->alloc * 2;
        s3devent *new_array = realloc(
            eq->array, sizeof(*eq->array) * newalloc
        );
        if (!new_array) {
            mutex_Release(eq->accesslock);
            return 0;
        }
        eq->array = new_array;
        eq->alloc = newalloc;
    }
    memcpy(&eq->array[eq->fill], eq, sizeof(*ev));
    mutex_Release(eq->accesslock);
    return 1;
}

S3DEXP int s3devent_q_IsEmpty(s3dequeue *eq) {
    int result = 0;
    mutex_Lock(eq->accesslock);
    result = (eq->fill == 0);
    mutex_Release(eq->accesslock);
    return result;
}

S3DEXP int s3devent_q_Pop(s3dequeue *eq, s3devent *writeto) {
    if (!eq)
        return 0;
    int result = 0;
    mutex_Lock(eq->accesslock);
    if (eq->fill == 0) {
        mutex_Release(eq->accesslock);
        return 0;
    }
    memcpy(writeto, &eq->array[0], sizeof(*writeto));
    if (eq->fill > 1) {
        memmove(&eq->array[0], &eq->array[1],
            sizeof(*eq->array) * (eq->fill - 1));
    }
    eq->fill -= 1;
    mutex_Release(eq->accesslock);
    return 1;
}

S3DEXP void s3devent_q_Free(s3dequeue *eq) {
    if (eq != NULL) {
        if (eq->accesslock != NULL) {
            mutex_Destroy(eq->accesslock);
        }
        free(eq->array);
    }
    free(eq);
}

S3DHID void thread_MarkAsMainThread(void);  // Used below.

#ifndef SPEW3D_OPTION_DISABLE_SDL
S3DHID int _spew3d_window_HandleSDLEvent(SDL_Event *e);
#endif

S3DEXP void s3devent_UpdateMainThread() {
    thread_MarkAsMainThread();
    #ifndef SPEW3D_OPTION_DISABLE_SDL
    SDL_Event e = {0};
    while (SDL_PollEvent(&e)) {
        _spew3d_window_HandleSDLEvent(&e);
    }
    #endif
    spew3d_window_MainThreadUpdate(); 
}

#endif  // SPEW3D_IMPLEMENTATION

