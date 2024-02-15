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

#ifdef SPEW3D_IMPLEMENTATION

#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>


typedef struct spew3d_imgload_job {
    char *path;
    int vfsflags;
    int keepalpha;

    int hasfinished, hasstarted, fserror, markeddeleted;

    void *pixels;
    uint64_t w,h;
} spew3d_imgload_job;

s3d_mutex *spew3d_imgload_mutex = NULL;
spew3d_imgload_job **job_queue = NULL;
uint64_t job_queue_len = 0;
uint64_t job_queue_alloc = 0;
s3d_threadinfo *_imgloader_process_thread = NULL;

__attribute__((constructor)) static void _createMutex() {
    spew3d_imgload_mutex = mutex_Create();
    if (!spew3d_imgload_mutex) {
        fprintf(stderr, "spew3d_imgload.c: error: FATAL, "
            "failed to create spew3d_imgload_mutex\n");
        _exit(1);
    }
}

static void _spew3d_FreeJob(spew3d_imgload_job *job) {
    if (!job)
        return;
    free(job->pixels);
    free(job);
}

static int spew3d_imgload_ProcessJob() {
    mutex_Lock(spew3d_imgload_mutex);
    if (job_queue_len == 0) {
        mutex_Release(spew3d_imgload_mutex);
        return 0;
    }

    spew3d_imgload_job *job = job_queue[0];
    if (job_queue_len > 1)
        memmove(
            &job_queue[0], &job_queue[1],
            sizeof(*job_queue) * (job_queue_len - 1)
        );
    job_queue_len--;
    #if defined(DEBUG_SPEW3D_TEXTURE)
    fprintf(stderr,
        "spew3d_imgload.c: debug: "
        "spew3d_imgload_ProcessJob(): "
        "processing job, remaining jobs in queue: %d "
        "[job %p]\n",
        job_queue_len, job);
    #endif
    assert(!job->hasstarted);
    assert(!job->markeddeleted);
    job->hasstarted = 1;
    mutex_Release(spew3d_imgload_mutex);

    assert(!job->hasfinished);
    int fserr = 0;
    char *imgcompressed = NULL;
    uint64_t imgcompressedlen = 0;
    if (!spew3d_vfs_FileToBytes(
            job->path, job->vfsflags, &fserr,
            &imgcompressed, &imgcompressedlen
            )) {
        #if defined(DEBUG_SPEW3D_TEXTURE)
        fprintf(stderr,
            "spew3d_imgload.c: debug: "
            "spew3d_imgload_ProcessJob(): "
            "failed to read disk data for "
            "texture: \"%s\" [job %p]\n",
            job->path, job);
        #endif

        mutex_Lock(spew3d_imgload_mutex);
        if (job->markeddeleted) {
            _spew3d_FreeJob(job);
            mutex_Release(spew3d_imgload_mutex);
            return 1;
        }        
        job->fserror = fserr;
        assert(job->fserror != FSERR_SUCCESS);
        job->hasfinished = 1;
        mutex_Release(spew3d_imgload_mutex);
        return 1;
    }
    #if defined(DEBUG_SPEW3D_TEXTURE)
    fprintf(stderr,
        "spew3d_imgload.c: debug: "
        "spew3d_imgload_ProcessJob(): "
        "decoding this texture: %s [job %p]\n",
        job->path, job);
    #endif

    int w = 0;
    int h = 0;
    int n = 0;
    unsigned char *data32 = stbi_load_from_memory(
        (unsigned char *)imgcompressed,
        imgcompressedlen, &w, &h, &n, 4
    );
    mutex_Lock(spew3d_imgload_mutex);
    if (job->markeddeleted) {
        if (data32) free(data32);
        _spew3d_FreeJob(job);
        mutex_Release(spew3d_imgload_mutex);
        return 1;
    }
    if (!data32) {
        #if defined(DEBUG_SPEW3D_TEXTURE)
        fprintf(stderr,
            "spew3d_imgload.c: debug: "
            "spew3d_imgload_ProcessJob(): "
            "failed to decode or allocate image "
            "for texture: \"%s\" [job %p]\n",
            job->path, job);
        #endif
        job->hasfinished = 1;
        assert(!job->pixels);
        job->fserror = FSERR_SUCCESS;
        mutex_Release(spew3d_imgload_mutex);
        return 1;
    }
    job->w = w;
    job->h = h;
    job->fserror = FSERR_SUCCESS;
    job->pixels = data32;
    job->hasfinished = 1;
    #if defined(DEBUG_SPEW3D_TEXTURE)
    fprintf(stderr,
        "spew3d_imgload.c: debug: "
        "spew3d_imgload_ProcessJob(): "
        "succeeded for texture: \"%s\" (size: "
        "%d,%d) [job %p]\n",
        job->path, (int)job->w, (int)job->h,
        job);
    #endif
    mutex_Release(spew3d_imgload_mutex);
    return 1;
}

void spew3d_imgload_DestroyJob(spew3d_imgload_job *job) {
    if (!job)
        return;
    mutex_Lock(spew3d_imgload_mutex);
    assert(!job->markeddeleted);
    if (job->hasfinished || !job->hasstarted) {
        if (!job->hasstarted) {
            int foundinlist = 0;
            int k = 0;
            while (k < job_queue_len) {
                if (job_queue[k] == job) {
                    assert(!foundinlist);
                    foundinlist = 1;
                    if (k + 1 < job_queue_len)
                        memmove(
                            &job_queue[k], &job_queue[k + 1],
                            sizeof(*job_queue) *
                                (job_queue_len - k - 1)
                        );
                    job_queue_len--;
                }
                k++;
            }
            assert(foundinlist);
        }
        _spew3d_FreeJob(job);
        mutex_Release(spew3d_imgload_mutex);
        return;
    }
    job->markeddeleted = 1;
    mutex_Release(spew3d_imgload_mutex);
}

static void _spew3d_imgload_JobThread(void* userdata) {
    while (1) {
        if (!spew3d_imgload_ProcessJob()) {
            // Since we had nothing to do, sleep a little bit:
            spew3d_time_Sleep(100);
            // (...so other stuff can run unimpeded.)
        }
    }
}

spew3d_imgload_job *spew3d_imgload_NewJob(
        const char *path, int vfsflags
        ) {
    mutex_Lock(spew3d_imgload_mutex);
    if (_imgloader_process_thread == NULL) {
        _imgloader_process_thread = (
            thread_SpawnWithPriority(
                S3DTHREAD_PRIO_LOW, _spew3d_imgload_JobThread,
                NULL
            )
        );
        if (!_imgloader_process_thread) {
            mutex_Release(spew3d_imgload_mutex);
            return NULL;
        }
    }
    mutex_Release(spew3d_imgload_mutex);

    spew3d_imgload_job *job = malloc(
        sizeof(spew3d_imgload_job)
    );
    if (!job) return NULL;
    memset(job, 0, sizeof(*job));
    job->path = strdup(path);
    if (!job->path) {
        free(job);
        return NULL;
    }
    job->vfsflags = vfsflags;
    if (job_queue_len + 1 > job_queue_alloc) {
        int new_alloc = job_queue_alloc * 2;
        if (new_alloc < 16)
            new_alloc = 16;
        spew3d_imgload_job **newqueue = realloc(
            job_queue, sizeof(*newqueue) * new_alloc
        );
        if (!newqueue) {
            free(job->path);
            free(job);
            return NULL;
        }
        job_queue = newqueue;
    }
    job_queue_len++;
    job_queue[job_queue_len - 1] = job;
    return job;
}

int spew3d_imgload_IsDone(spew3d_imgload_job *job) {
    mutex_Lock(spew3d_imgload_mutex);
    assert(!job->markeddeleted);
    if (!job->hasfinished) {
        mutex_Release(spew3d_imgload_mutex);
        return 0;
    }
    mutex_Release(spew3d_imgload_mutex);
    return 1;
}

int spew3d_imgload_GetResult(
        spew3d_imgload_job *job, void **out_pixels,
        int *out_w, int *out_h, int *out_fserr
        ) {
    mutex_Lock(spew3d_imgload_mutex);
    assert(!job->markeddeleted);
    assert(job->hasfinished);
    if (job->fserror != FSERR_SUCCESS) {
        assert(!job->pixels);
        if (out_fserr)
            *out_fserr = job->fserror;
        *out_pixels = NULL;
        *out_w = 0;
        *out_h = 0;
        mutex_Release(spew3d_imgload_mutex);
        return 0;
    }
    assert(job->pixels);
    if (out_fserr)
        *out_fserr = FSERR_SUCCESS;
    *out_pixels = job->pixels;
    *out_w = job->w;
    *out_h = job->h;
    mutex_Release(spew3d_imgload_mutex);
    return 1;
}

#endif  // SPEW3D_IMPLEMENTATION

