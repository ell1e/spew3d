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

#ifndef SPEW3D_THREADING_H_
#define SPEW3D_THREADING_H_

typedef struct s3d_mutex s3d_mutex;

typedef struct s3d_semaphore s3d_semaphore;

typedef struct s3d_threadinfo s3d_threadinfo;

typedef struct s3d_tevent s3d_tevent;

S3DEXP s3d_semaphore *semaphore_Create(int value);

S3DEXP void semaphore_Wait(s3d_semaphore *s);

S3DEXP void semaphore_Post(s3d_semaphore *s);

S3DEXP void semaphore_Destroy(s3d_semaphore *s);

S3DEXP s3d_mutex *mutex_Create();

S3DEXP void mutex_Lock(s3d_mutex *m);

S3DEXP int mutex_TryLock(s3d_mutex *m);

S3DEXP int mutex_IsLocked(s3d_mutex *m);

S3DEXP int mutex_TryLockWithTimeout(
    s3d_mutex *m, int32_t timeoutms
);

S3DEXP void mutex_Release(s3d_mutex *m);

S3DEXP void mutex_Destroy(s3d_mutex *m);

S3DEXP s3d_threadinfo *thread_Spawn(
    void (*func)(void *userdata),
    void *userdata
);

#define S3DTHREAD_PRIO_LOW 1
#define S3DTHREAD_PRIO_NORMAL 2
#define S3DTHREAD_PRIO_HIGH 3

S3DEXP s3d_threadinfo *thread_SpawnWithPriority(
    int priority,
    void (*func)(void* userdata), void *userdata
);

S3DEXP s3d_tevent *threadevent_Create();

S3DEXP void threadevent_Free(s3d_tevent *e);

S3DEXP void thread_Detach(s3d_threadinfo *t);

S3DEXP void thread_Join(s3d_threadinfo *t);

S3DEXP int thread_InMainThread();

S3DEXP void threadevent_Wait(s3d_tevent *e);

S3DEXP void threadevent_Set(s3d_tevent *e);

#endif  // SPEW3D_THREADING_H_
