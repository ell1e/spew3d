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

#if defined(SPEW3D_IMPLEMENTATION) && \
    SPEW3D_IMPLEMENTATION != 0

#include <string.h>

typedef struct s3d_equeue {
    int fill, alloc;
    s3d_event *array;
    s3d_mutex *accesslock;
} s3d_equeue;

S3DEXP s3d_equeue *spew3d_event_q_Create() {
    s3d_equeue *eq = malloc(sizeof(*eq));
    if (!eq)
        return NULL;
    memset(eq, 0, sizeof(*eq));
    eq->alloc = 1024;
    eq->array = malloc(sizeof(*eq->array) * eq->alloc);
    if (!eq->array) {
        free(eq);
        return NULL;
    }
    memset(eq->array, 0, sizeof(*eq->array) * eq->alloc);
    eq->accesslock = mutex_Create();
    if (!eq->accesslock) {
        free(eq->array);
        free(eq);
        return NULL;
    }
    return eq;
}

s3d_equeue *_main_event_queue = NULL;
s3d_equeue *_internal_event_queue = NULL;

S3DHID __attribute__((constructor)) static void _make_main_queue() {
    if (_main_event_queue != NULL &&
            _internal_event_queue != NULL)
        return;
    if (!_main_event_queue)
        _main_event_queue = spew3d_event_q_Create();
    if (!_internal_event_queue)
        _internal_event_queue = spew3d_event_q_Create();
    if (!_main_event_queue || !_internal_event_queue) {
        fprintf(stderr, "spew3d_event.c: error: "
            "Failed to allocate event queues.\n");
        _exit(1);
    }
}

S3DEXP s3d_equeue *spew3d_event_GetMainQueue() {
    _make_main_queue();
    return _main_event_queue;
}

S3DHID s3d_equeue *_spew3d_event_GetInternalQueue() {
    _make_main_queue();
    return _internal_event_queue;
}

S3DHID void _spew3d_event_q_InsertForce(
        s3d_equeue *eq, const s3d_event *ev
        ) {
    while (1) {
        if (spew3d_event_q_Insert(eq, ev))
            return;

        spew3d_time_Sleep(20);
    }
}

S3DEXP int spew3d_event_q_Insert(s3d_equeue *eq, const s3d_event *ev) {
    mutex_Lock(eq->accesslock);
    if (eq->fill + 1 > eq->alloc) {
        int newalloc = eq->alloc * 2;
        s3d_event *new_array = realloc(
            eq->array, sizeof(*eq->array) * newalloc
        );
        if (!new_array) {
            mutex_Release(eq->accesslock);
            return 0;
        }
        eq->array = new_array;
        eq->alloc = newalloc;
    }
    #if defined(DEBUG_SPEW3D_EVENT)
    if (!S3DEV_TYPE_IS_INTERNAL(ev->kind)) {
        printf("spew3d_event.c: debug: "
            "Inserted non-internal event (queue %p): "
            "event kind %d\n",
            eq, (int)ev->kind);
    }
    #endif
    memcpy(&eq->array[eq->fill], ev, sizeof(*ev));
    assert(eq->array[eq->fill].kind == ev->kind);
    eq->fill++;
    mutex_Release(eq->accesslock);
    return 1;
}

S3DEXP int spew3d_event_q_IsEmpty(s3d_equeue *eq) {
    int result = 0;
    mutex_Lock(eq->accesslock);
    result = (eq->fill == 0);
    mutex_Release(eq->accesslock);
    return result;
}

S3DEXP int spew3d_event_q_Pop(s3d_equeue *eq, s3d_event *writeto) {
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

S3DEXP void spew3d_event_q_Free(s3d_equeue *eq) {
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

S3DHID void spew3d_audio_mixer_UpdateAllOnMainThread();

S3DEXP void spew3d_event_UpdateMainThread() {
    thread_MarkAsMainThread();
    #ifndef SPEW3D_OPTION_DISABLE_SDL
    SDL_Event e = {0};
    while (SDL_PollEvent(&e)) {
        _spew3d_window_HandleSDLEvent(&e);
    }
    #endif
    s3d_equeue *eq = _spew3d_event_GetInternalQueue();
    assert(eq != NULL);

    while (1) {
        spew3d_audio_mixer_InternalUpdateAllOnMainThread();
        spew3d_audio_sink_InternalMainThreadUpdate();
        spew3d_window_InternalMainThreadUpdate();

        s3d_event e = {0};
        if (!spew3d_event_q_Pop(eq, &e))
            break;

        assert(e.kind != S3DEV_INVALID);
        if (!spew3d_window_InternalMainThreadProcessEvent(&e)) {
            if (!spew3d_texture_InternalMainThreadProcessEvent(&e))
                spew3d_camera_InternalMainThreadProcessEvent(&e);
        }
    }
}

#endif  // SPEW3D_IMPLEMENTATION

