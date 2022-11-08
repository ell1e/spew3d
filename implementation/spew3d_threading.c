/* Copyright (c) 2020-2022, ellie/@ell1e & Spew3D Team (see AUTHORS.md).

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

#ifndef WINDOWS
#if defined(WIN32) || defined(_WIN32) || defined(_WIN64) || defined(__WIN32__)
#define WINDOWS
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif
#if defined __MINGW_H
#define _WIN32_IE 0x0400
#endif
#endif
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <inttypes.h>
#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#else
#include <pthread.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sched.h>
#endif
#if defined(__linux__) || defined(linux) || defined(__linux)
#include <linux/sched.h>
#endif


#include <assert.h>
#include <stdlib.h>
#include <string.h>
#ifdef WINDOWS
#include <process.h>
#include <windows.h>
#else
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#endif


typedef struct s3d_mutex {
#ifdef WINDOWS
    HANDLE m;
#else
    pthread_mutex_t m;
#endif
} s3d_mutex;


typedef struct s3d_tevent {
#ifdef WINDOWS
    HANDLE e;
#else
    char was_set;
    pthread_cond_t e;
    pthread_mutex_t m;
#endif
} s3d_tevent;

typedef struct s3d_semaphore {
#ifdef WINDOWS
    HANDLE s;
#else
#if defined(__APPLE__) || defined(__OSX__)
    sem_t *s;
    char *sname;
#else
    sem_t s;
#endif
#endif
} s3d_semaphore;


typedef struct s3d_threadinfo {
#ifdef WINDOWS
    HANDLE t;
#else
    pthread_t t;
#endif
} s3d_threadinfo;


#if defined(__APPLE__) || defined(__OSX__)
uint64_t _last_sem_id = 0;
s3d_mutex *synchronize_sem_id = NULL;
uint64_t uniquesemidpart = 0;
__attribute__((constructor)) void __init_semId_synchronize() {
    synchronize_sem_id = s3d_mutex_Create();
    secrandom_GetBytes(
        (char*)&uniquesemidpart, sizeof(uniquesemidpart)
    );
}
#endif


s3d_semaphore *semaphore_Create(int value) {
    s3d_semaphore *s = malloc(sizeof(*s));
    if (!s) {
        return NULL;
    }
    memset(s, 0, sizeof(*s));
#ifdef WINDOWS
    s->s = CreateSemaphore(NULL, value, INT_MAX, NULL);
    if (!s->s) {
        free(s);
        return NULL;
    }
#elif defined(__APPLE__) || defined(__OSX__)
    mutex_Lock(synchronize_sem_id);
    _last_sem_id++;
    uint64_t semid = _last_sem_id;
    mutex_Release(synchronize_sem_id);
    char sem_name[128];
    snprintf(
        sem_name, sizeof(sem_name) - 1,
        "/s3dsemaphore%" PRIu64 "id%" PRIu64,
        uniquesemidpart, semid
    );
    s->sname = strdup(sem_name);
    if (!s->sname) {
        free(s);
        return NULL;
    }
    s->s = sem_open(sem_name, O_CREAT, 0600, value);
    if (s->s == SEM_FAILED) {
        free(s->sname);
        free(s);
        return NULL;
    }
#else
    if (sem_init(&s->s, 0, value) != 0) {
        free(s);
        return NULL;
    }
#endif
    return s;
}


void semaphore_Wait(s3d_semaphore *s) {
#ifdef WINDOWS
    WaitForSingleObject(s->s, INFINITE);
#else
#if defined(__APPLE__) || defined(__OSX__)
    sem_wait(s->s);
#else
    sem_wait(&s->s);
#endif
#endif
}


void semaphore_Post(s3d_semaphore *s) {
#ifdef WINDOWS
    ReleaseSemaphore(s->s, 1, NULL);
#else
#if defined(__APPLE__) || defined(__OSX__)
    sem_post(s->s);
#else
    sem_post(&s->s);
#endif
#endif
}


void semaphore_Destroy(s3d_semaphore *s) {
    if (!s) {
        return;
    }
#ifdef WINDOWS
    CloseHandle(s->s);
#else
#if defined(__APPLE__) || defined(__OSX__)
    sem_unlink(s->sname);
    free(s->sname);
#else
    sem_destroy(&s->s);
#endif
#endif
    free(s);
}


s3d_mutex *mutex_Create() {
    s3d_mutex* m = malloc(sizeof(*m));
    if (!m) {
        return NULL;
    }
    memset(m, 0, sizeof(*m));
#ifdef WINDOWS
    m->m = CreateMutex(NULL, FALSE, NULL);
    if (!m->m) {
        free(m);
        return NULL;
    }
#else
    pthread_mutexattr_t blub;
    pthread_mutexattr_init(&blub);
#ifndef NDEBUG
    pthread_mutexattr_settype(&blub, PTHREAD_MUTEX_ERRORCHECK);
#else
    pthread_mutexattr_settype(&blub, PTHREAD_MUTEX_NORMAL);
#endif
    while (pthread_mutex_init(&m->m, &blub) != 0) {
        if (errno != EAGAIN) {
            free(m);
            return NULL;
        }
    }
    pthread_mutexattr_destroy(&blub);
#endif
    return m;
}


void mutex_Destroy(s3d_mutex *m) {
    if (!m) {
        return;
    }
#ifdef WINDOWS
    CloseHandle(m->m);
#else
    while (pthread_mutex_destroy(&m->m) != 0) {
        if (errno != EBUSY) {
            break;
        }
    }
#endif
    free(m);
}


void mutex_Lock(s3d_mutex *m) {
#ifdef WINDOWS
    WaitForSingleObject(m->m, INFINITE);
#else
#ifndef NDEBUG
    assert(pthread_mutex_lock(&m->m) == 0);
#else
    pthread_mutex_lock(&m->m);
#endif
#endif
}

int mutex_TryLock(s3d_mutex *m) {
#ifdef WINDOWS
    if (WaitForSingleObject(m->m, 0) == WAIT_OBJECT_0) {
        return 1;
    }
    return 0;
#else
    if (pthread_mutex_trylock(&m->m) != 0) {
        return 0;
    }
    return 1;
#endif
}


int mutex_TryLockWithTimeout(s3d_mutex *m, int32_t timeoutms) {
    if (timeoutms <= 0)
        return mutex_TryLock(m);
#ifdef WINDOWS
    if (WaitForSingleObject(m->m, (DWORD)timeoutms) == WAIT_OBJECT_0) {
        return 1;
    }
    return 0;
#else
    struct timespec msspec;
    clock_gettime(CLOCK_REALTIME, &msspec);
    msspec.tv_nsec += (int32_t)1000 * timeoutms;
    if (pthread_mutex_timedlock(&m->m, &msspec) != 0) {
        return 0;
    }
    return 1;
#endif
}


void mutex_Release(s3d_mutex *m) {
#ifdef WINDOWS
    ReleaseMutex(m->m);
#else
#ifndef NDEBUG
    //int i = pthread_mutex_unlock(&m->m);
    //printf("return value: %d\n", i);
    //assert(i == 0);
    assert(pthread_mutex_unlock(&m->m) == 0);
#else
    pthread_mutex_unlock(&m->m);
#endif
#endif
}


void thread_Detach(s3d_threadinfo *t) {
#ifdef WINDOWS
    CloseHandle(t->t);
#else
    pthread_detach(t->t);
#endif
    free(t);
}


struct spawninfo {
    void (*func)(void* userdata);
    void* userdata;
};


#ifdef WINDOWS
static unsigned __stdcall spawnthread(void* data) {
#else
static void* spawnthread(void* data) {
#endif
    struct spawninfo* sinfo = data;
    sinfo->func(sinfo->userdata);
    free(sinfo);
#ifdef WINDOWS
    return 0;
#else
    return NULL;
#endif
}


s3d_threadinfo *thread_Spawn(
        void (*func)(void *userdata),
        void *userdata
        ) {
    return thread_SpawnWithPriority(
        THREAD_PRIO_NORMAL, func, userdata
    );
}


s3d_threadinfo *thread_SpawnWithPriority(
        int priority,
        void (*func)(void* userdata), void *userdata
        ) {
    struct spawninfo* sinfo = malloc(sizeof(*sinfo));
    if (!sinfo)
        return NULL;
    memset(sinfo, 0, sizeof(*sinfo));
    sinfo->func = func;
    sinfo->userdata = userdata;
    s3d_threadinfo *t = malloc(sizeof(*t));
    if (!t) {
        free(sinfo);
        return NULL;
    }
    memset(t, 0, sizeof(*t));
#ifdef WINDOWS
    HANDLE h = (HANDLE)_beginthreadex(NULL, 0, spawnthread, sinfo, 0, NULL);
    t->t = h;
#else
    while (pthread_create(&t->t, NULL, spawnthread, sinfo) != 0) {
        assert(errno == EAGAIN);
    }
    #if defined(__linux__) || defined(__LINUX__)
    if (priority == 0) {
        struct sched_param param;
        memset(&param, 0, sizeof(param));
        param.sched_priority = sched_get_priority_min(SCHED_BATCH);
        pthread_setschedparam(t->t, SCHED_BATCH, &param);
    }
    if (priority == 2) {
        struct sched_param param;
        int policy;
        pthread_getschedparam(t->t, &policy, &param);
        param.sched_priority = sched_get_priority_max(policy);
        pthread_setschedparam(t->t, policy, &param);
    }
    #endif
#endif
    return t;
}


#ifndef WINDOWS
static pthread_t mainThread;
#else
static DWORD mainThread;
#endif


__attribute__((constructor)) void thread_MarkAsMainThread(void) {
    // mark current thread as main thread
#ifndef WINDOWS
    mainThread = pthread_self();
#else
    mainThread = GetCurrentThreadId();
#endif
}


int thread_InMainThread() {
#ifndef WINDOWS
    return (pthread_self() == mainThread);
#else
    return (GetCurrentThreadId() == mainThread);
#endif
}


void thread_Join(s3d_threadinfo *t) {
#ifdef WINDOWS
    WaitForMultipleObjects(1, &t->t, TRUE, INFINITE);
    CloseHandle(t->t);
#else
    pthread_join(t->t, NULL);
#endif
    free(t);
}


s3d_tevent *threadevent_Create() {
    s3d_tevent *e = malloc(sizeof(*e));
    if (!e)
        return NULL;
    memset(e, 0, sizeof(*e));
#ifdef WINDOWS
    e->e = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (e->e == NULL) {
        free(e);
        return NULL;
    }
#else
    pthread_mutex_init(&e->m, NULL);
    pthread_cond_init(&e->e, NULL);
#endif
    return e;
}


void threadevent_Free(s3d_tevent *e) {
#ifdef WINDOWS
    if (e) {
        CloseHandle(e->e);
    }
#else
    if (e) {
        pthread_mutex_destroy(&e->m);
        pthread_cond_destroy(&e->e);
    }
#endif
    free(e);
}


void threadevent_Wait(s3d_tevent *e) {
#ifdef WINDOWS
    WaitForSingleObject(e->e);
#else
    pthread_mutex_lock(&e->m);
    while (!e->was_set) {
        pthread_cond_wait(&e->e, &e->m);
    }
    e->was_set = 0;
    pthread_mutex_unlock(&e->m);
#endif
}


void threadevent_Set(s3d_tevent *e) {
#ifdef WINDOWS
    SetEvent(e->e);
#else
    pthread_mutex_lock(&e->m);
    e->was_set = 1;
    pthread_mutex_unlock(&e->m);
    pthread_cond_signal(&e->e);
#endif
}

#endif  // SPEW3D_IMPLEMENTATION

