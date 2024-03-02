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

#ifdef SPEW3D_IMPLEMENTATION

#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

typedef struct s3d_resourceload_job {
    char *path;
    int vfsflags;
    int rltype;
    void *extradata;
    void *(*callback)(const char *path, int vfsflags,
        void *extradata);

    int hasfinished, hasstarted, fserror,
        nonfserror, markeddeleted;

    s3d_resourceload_result result;
} s3d_resourceload_job;

s3d_mutex *_spew3d_resourceload_mutex = NULL;
s3d_resourceload_job **job_queue = NULL;
uint64_t job_queue_len = 0;
uint64_t job_queue_alloc = 0;
s3d_threadinfo *_imgloader_process_thread = NULL;

S3DHID __attribute__((constructor)) static void _createMutex() {
    _spew3d_resourceload_mutex = mutex_Create();
    if (!_spew3d_resourceload_mutex) {
        fprintf(stderr, "spew3d_resourceload.c: error: FATAL, "
            "failed to create _spew3d_resourceload_mutex\n");
        _exit(1);
    }
}

S3DHID static void _s3d_resourceload_FreeJob(
        s3d_resourceload_job *job) {
    if (!job)
        return;
    if (job->rltype == RLTYPE_IMAGE) {
        free(job->result.resource_image.pixels);
    }
    free(job);
}

S3DHID static int s3d_resourceload_ProcessJob() {
    mutex_Lock(_spew3d_resourceload_mutex);
    if (job_queue_len == 0) {
        mutex_Release(_spew3d_resourceload_mutex);
        return 0;
    }

    s3d_resourceload_job *job = job_queue[0];
    if (job_queue_len > 1)
        memmove(
            &job_queue[0], &job_queue[1],
            sizeof(*job_queue) * (job_queue_len - 1)
        );
    job_queue_len--;
    #if defined(DEBUG_SPEW3D_RESOURCELOAD)
    fprintf(stderr,
        "spew3d_resourceload.c: debug: "
        "s3d_resourceload_ProcessJob(): "
        "processing job, remaining jobs in queue: %d "
        "[job %p]\n",
        job_queue_len, job);
    #endif
    assert(!job->hasstarted);
    assert(!job->markeddeleted);
    int rltype = job->rltype;
    job->hasstarted = 1;
    mutex_Release(_spew3d_resourceload_mutex);

    assert(!job->hasfinished);
    if (rltype == RLTYPE_IMAGE) {
        int fserr = 0;
        char *imgcompressed = NULL;
        uint64_t imgcompressedlen = 0;
        if (!spew3d_vfs_FileToBytes(
                job->path, job->vfsflags, &fserr,
                &imgcompressed, &imgcompressedlen
                )) {
            #if defined(DEBUG_SPEW3D_RESOURCELOAD)
            fprintf(stderr,
                "spew3d_resourceload.c: debug: "
                "s3d_resourceload_ProcessJob(): "
                "failed to read disk data for "
                "texture: \"%s\" [job %p]\n",
                job->path, job);
            #endif

            mutex_Lock(_spew3d_resourceload_mutex);
            if (job->markeddeleted) {
                _s3d_resourceload_FreeJob(job);
                mutex_Release(_spew3d_resourceload_mutex);
                return 1;
            }
            job->fserror = fserr;
            assert(job->fserror != FSERR_SUCCESS);
            job->hasfinished = 1;
            mutex_Release(_spew3d_resourceload_mutex);
            return 1;
        }
        #if defined(DEBUG_SPEW3D_RESOURCELOAD)
        fprintf(stderr,
            "spew3d_resourceload.c: debug: "
            "s3d_resourceload_ProcessJob(): "
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
        free(imgcompressed);
        mutex_Lock(_spew3d_resourceload_mutex);
        if (job->markeddeleted) {
            if (data32) free(data32);
            _s3d_resourceload_FreeJob(job);
            mutex_Release(_spew3d_resourceload_mutex);
            return 1;
        }
        if (!data32) {
            #if defined(DEBUG_SPEW3D_RESOURCELOAD)
            fprintf(stderr,
                "spew3d_resourceload.c: debug: "
                "s3d_resourceload_ProcessJob(): "
                "Failed to decode or allocate image "
                "for texture: \"%s\" [job %p]\n",
                job->path, job);
            #endif
            job->hasfinished = 1;
            assert(job->rltype != RLTYPE_IMAGE ||
                job->result.resource_image.pixels == NULL);
            job->nonfserror = 1;
            job->fserror = FSERR_SUCCESS;
            mutex_Release(_spew3d_resourceload_mutex);
            return 1;
        }
        job->result.resource_image.w = w;
        job->result.resource_image.h = h;
        job->fserror = FSERR_SUCCESS;
        job->result.resource_image.pixels = data32;
        job->hasfinished = 1;
        #if defined(DEBUG_SPEW3D_RESOURCELOAD)
        fprintf(stderr,
            "spew3d_resourceload.c: debug: "
            "s3d_resourceload_ProcessJob(): "
            "Succeeded for texture: \"%s\" (size: "
            "%d,%d) [job %p]\n",
            job->path, (int)job->w, (int)job->h,
            job);
        #endif
        mutex_Release(_spew3d_resourceload_mutex);
    } else if (job->callback != NULL) {
        mutex_Lock(_spew3d_resourceload_mutex);
        char *path = job->path != NULL ? strdup(job->path) : NULL;
        int vfsflags = job->vfsflags;
        void *extradata = job->extradata;
        void *(*cb)(const char *path,
                int vfsflags, void *extradata) =
            job->callback;
        if (path == NULL && job->path != NULL) {
             fprintf(stderr,
                "spew3d_resourceload.c: warning: "
                "s3d_resourceload_ProcessJob(): "
                "Failed to process job with "
                "path \"%s\" [job %p], out of memory.\n",
                rltype, job->path, job);
            job->hasfinished = 1;
            job->nonfserror = 1;
            job->fserror = FSERR_SUCCESS;
            mutex_Release(_spew3d_resourceload_mutex);
            return 1;
        }
        mutex_Release(_spew3d_resourceload_mutex);
        void *result = cb(path, vfsflags, extradata);
        mutex_Lock(_spew3d_resourceload_mutex);
        if (!result) {
            #if defined(DEBUG_SPEW3D_RESOURCELOAD)
            fprintf(stderr,
                "spew3d_resourceload.c: debug: "
                "s3d_resourceload_ProcessJob(): "
                "Callback returned failure for generic job: "
                "\"%s\" [job %p]\n",
                job->path, job);
            #endif
            job->hasfinished = 1;
            job->nonfserror = 1;
            job->fserror = FSERR_SUCCESS;
            mutex_Release(_spew3d_resourceload_mutex);
            return 1;
        }
        job->result.generic.callback_result = result;
        job->hasfinished = 1;
        job->nonfserror = 0;
        job->fserror = FSERR_SUCCESS;
        #if defined(DEBUG_SPEW3D_RESOURCELOAD)
        fprintf(stderr,
            "spew3d_resourceload.c: debug: "
            "s3d_resourceload_ProcessJob(): "
            "Succeeded for generic job: \"%s\" (size: "
            "%d,%d) [job %p]\n",
            job->path, (int)job->w, (int)job->h,
            job);
        #endif
        mutex_Release(_spew3d_resourceload_mutex);
    } else {
        mutex_Lock(_spew3d_resourceload_mutex);
        fprintf(stderr,
            "spew3d_resourceload.c: warning: "
            "s3d_resourceload_ProcessJob(): "
            "Failed to process job of unknown "
            "type %d, path \"%s\" [job %p]\n",
            rltype, job->path, job);
        job->hasfinished = 1;
        
        assert(job->rltype != RLTYPE_IMAGE ||
            job->result.resource_image.pixels == NULL);
        job->nonfserror = 1;
        job->fserror = FSERR_SUCCESS;
        mutex_Release(_spew3d_resourceload_mutex);
    }
    return 1;
}

S3DEXP void s3d_resourceload_DestroyJob(
        s3d_resourceload_job *job) {
    if (!job)
        return;
    mutex_Lock(_spew3d_resourceload_mutex);
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
        _s3d_resourceload_FreeJob(job);
        mutex_Release(_spew3d_resourceload_mutex);
        return;
    }
    job->markeddeleted = 1;
    mutex_Release(_spew3d_resourceload_mutex);
}

S3DHID static void _s3d_resourceload_JobThread(
        void* userdata) {
    while (1) {
        if (!s3d_resourceload_ProcessJob()) {
            // Since we had nothing to do, sleep a little bit:
            spew3d_time_Sleep(100);
            // (...so other stuff can run unimpeded.)
        }
    }
}

S3DEXP s3d_resourceload_job *s3d_resourceload_NewJob(
        const char *path, int rltype, int vfsflags
        ) {
    return s3d_resourceload_NewJobWithCallback(
        path, rltype, vfsflags, NULL, NULL
    );
}

S3DEXP s3d_resourceload_job *s3d_resourceload_NewJobWithCallback(
        const char *path, int rltype, int vfsflags,
        void *(*callback)(const char *path, int vfsflags,
            void *extradata),
        void *extradata
        ) {
    mutex_Lock(_spew3d_resourceload_mutex);
    if (_imgloader_process_thread == NULL) {
        _imgloader_process_thread = (
            thread_SpawnWithPriority(
                S3DTHREAD_PRIO_LOW, _s3d_resourceload_JobThread,
                NULL
            )
        );
        if (!_imgloader_process_thread) {
            mutex_Release(_spew3d_resourceload_mutex);
            return NULL;
        }
    }
    mutex_Release(_spew3d_resourceload_mutex);

    s3d_resourceload_job *job = malloc(
        sizeof(s3d_resourceload_job)
    );
    if (!job) return NULL;
    memset(job, 0, sizeof(*job));
    job->extradata = extradata;
    job->callback = callback;
    job->path = strdup(path);
    if (!job->path) {
        free(job);
        return NULL;
    }
    job->rltype = rltype;
    job->vfsflags = vfsflags;
    if (job_queue_len + 1 > job_queue_alloc) {
        int new_alloc = job_queue_alloc * 2;
        if (new_alloc < 16)
            new_alloc = 16;
        s3d_resourceload_job **newqueue = realloc(
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

S3DEXP int s3d_resourceload_IsDone(
        s3d_resourceload_job *job) {
    mutex_Lock(_spew3d_resourceload_mutex);
    assert(!job->markeddeleted);
    if (!job->hasfinished) {
        mutex_Release(_spew3d_resourceload_mutex);
        return 0;
    }
    mutex_Release(_spew3d_resourceload_mutex);
    return 1;
}

S3DEXP int s3d_resourceload_ExtractResult(
        s3d_resourceload_job *job,
        s3d_resourceload_result *out_result,
        int *out_fserr
        ) {
    mutex_Lock(_spew3d_resourceload_mutex);
    assert(!job->markeddeleted);
    assert(job->hasfinished);
    if (job->fserror != FSERR_SUCCESS ||
            job->nonfserror) {
        assert(job->rltype != RLTYPE_IMAGE ||
            job->result.resource_image.pixels == NULL);
        if (out_fserr)
            *out_fserr = job->fserror;
        memset(out_result, 0, sizeof(*out_result));
        mutex_Release(_spew3d_resourceload_mutex);
        return 0;
    }
    assert(job->rltype != RLTYPE_IMAGE ||
        job->result.resource_image.pixels != NULL);
    if (out_fserr)
        *out_fserr = FSERR_SUCCESS;
    memcpy(out_result, &job->result, sizeof(*out_result));
    memset(  // Important, since caller owns result now.
        &job->result, 0, sizeof(job->result)
    );
    mutex_Release(_spew3d_resourceload_mutex);
    return 1;
}

#endif  // SPEW3D_IMPLEMENTATION

