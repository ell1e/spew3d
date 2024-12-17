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

#include <assert.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Job struct used in itemsort_Do:
typedef struct _itemsort_quicksortjob {
    int64_t start, end;
} _itemsort_quicksortjob;

typedef struct s3d_sortstructcache {
    _itemsort_quicksortjob *cachedjobs;
    int64_t cachedjobsalloc;
} s3d_sortstructcache;

// Helper macro for itemsort_Do:
#define SORT_PUSHJOB(start_idx, end_idx) \
    if (end_idx > start_idx) {\
        if (jobs_count + 1 > jobs_alloc) {\
            int64_t new_jobs_alloc = jobs_alloc * 32;\
            if (new_jobs_alloc < jobs_count + 16)\
                new_jobs_alloc = jobs_count + 16;\
            _itemsort_quicksortjob *jobs_new = NULL;\
            if (jobs_areonheap) {\
                jobs_new = realloc(\
                    jobs,\
                    sizeof(*jobs) * new_jobs_alloc\
                );\
            } else {\
                jobs_new = malloc(\
                    sizeof(*jobs) * new_jobs_alloc\
                );\
                if (jobs_new && jobs_count > 0)\
                    memcpy(jobs_new, jobs,\
                        sizeof(*jobs) * jobs_count);\
            }\
            if (!jobs_new) {\
                goto exitoom;\
            }\
            jobs_areonheap = 1;\
            jobs = jobs_new;\
        }\
        memset(&jobs[jobs_count], 0, sizeof(*jobs));\
        jobs[jobs_count].start = start_idx;\
        jobs[jobs_count].end = end_idx;\
        jobs_count++;\
    }\

// Helper macro for itemsort_Do:
#define SORT_GETITEM(idx) \
    (\
        ((char*) sortdata) + itemsize * (int64_t)(idx)\
    )

S3DEXP s3d_sortstructcache *s3d_itemsort_CreateCache() {
    s3d_sortstructcache *cache = malloc(sizeof(*cache));
    if (!cache)
        return NULL;
    memset(cache, 0, sizeof(*cache));
    return cache;
}

S3DEXP void s3d_itemsort_FreeCache(s3d_sortstructcache *cache) {
    if (!cache)
        return;
    if (cache->cachedjobs)
        free(cache->cachedjobs);
    free(cache);
}

S3DEXP int s3d_itemsort_Do(
        void *sortdata, int64_t sortdatabytes, int64_t itemsize,
        int (*compareFunc)(void *item1, void *item2),
        s3d_sortstructcache *cache,
        int *out_erroroom, int *out_errorunsortable
        ) {
    /// This function is currently based on quick sort with heap
    /// jobs (so no C stack recursion), and crypto random pivot.
    /// Should be reworked to fall back to some other algo if taking
    /// too many iterations, after which faster pseudo random pivots
    /// could be used.

    if (out_erroroom) *out_erroroom = 0;
    if (out_errorunsortable) *out_errorunsortable = 0;
    if (sortdatabytes <= itemsize)
        return 1;
    int64_t itemcount = (sortdatabytes / itemsize);

    // Job queue which starts on stack, migrates to heap if too large:
    _itemsort_quicksortjob _jobbuf[8];
    _itemsort_quicksortjob *jobs = _jobbuf;
    int64_t jobs_alloc = (
        sizeof(_jobbuf) / sizeof(_jobbuf[0])
    );
    int64_t jobs_areonheap = 0;
    int64_t jobs_count = 0;
    if (cache->cachedjobs != NULL) {
        jobs = cache->cachedjobs;
        jobs_areonheap = 1;
        jobs_alloc = cache->cachedjobsalloc;
    }

    char switchbuf[512];
    if (itemsize > (int64_t)sizeof(switchbuf)) {
        if (out_erroroom) *out_erroroom = 1;
        if (jobs_areonheap) {
            if (cache != NULL) {
                cache->cachedjobs = jobs;
                cache->cachedjobsalloc = jobs_alloc;
            } else {
                free(jobs);
            }
        }
        return 0;
    }

    // Add a job for entire range before we start:
    SORT_PUSHJOB(0, itemcount);
    assert(jobs_count > 0);

    // Now go through sort jobs:
    int64_t k = 0;
    while (k < jobs_count) {
        // Evaluate the sub range we want to sort in this job:
        int64_t curr_start = jobs[k].start;
        int64_t curr_end = jobs[k].end;
        assert(curr_start <= curr_end);
        assert(curr_start >= 0);
        assert(curr_end * itemsize <= sortdatabytes);
        if (curr_end - curr_start <= 1) {
            k++;
            continue;
        }
        if (curr_end == curr_start + 2) {
            // It's just two elements to sort, so do direct comparison:
            int cmp_1to2 = compareFunc(
                SORT_GETITEM(curr_start),
                SORT_GETITEM(curr_start + 1)
            );
            if (cmp_1to2 < -1) {
                if (cmp_1to2 == S3D_SORT_ERR_UNSORTABLE) {
                    exitunsortable: ;
                    if (out_errorunsortable) *out_errorunsortable = 1;
                    if (jobs_areonheap) {
                        if (cache != NULL) {
                            cache->cachedjobs = jobs;
                            cache->cachedjobsalloc = jobs_alloc;
                        } else {
                            free(jobs);
                        }
                    }
                    return 0;
                }
                assert(cmp_1to2 == S3D_SORT_ERR_OOM);
                exitoom: ;
                if (out_erroroom) *out_erroroom = 1;
                if (jobs_areonheap) {
                    if (cache != NULL) {
                        cache->cachedjobs = jobs;
                        cache->cachedjobsalloc = jobs_alloc;
                    } else {
                        free(jobs);
                    }
                }
                return 0;
            }
            if (cmp_1to2 > 0) {
                // Need to switch.
                memcpy(switchbuf, SORT_GETITEM(curr_start + 1),
                       itemsize);
                memcpy(SORT_GETITEM(curr_start + 1),
                       SORT_GETITEM(curr_start), itemsize);
                memcpy(SORT_GETITEM(curr_start), switchbuf, itemsize);
            }
            k++;
            continue;
        }
        int64_t pivot = spew3d_secrandom_RandIntRange(
            curr_start, curr_end - 1
            // (curr_end is exclusive, but
            // spew3d_secrandom_RandIntRange uses an
            // inclusive end)
        );  // for now sec random to avoid easily exploiting worst cases
        assert(pivot >= curr_start && pivot <= curr_end);
        
        // Go through the entire sub range we're sorting, and
        // move things according to our pivot (lower elements to below,
        // higher elements to above);
        int64_t z = curr_start;
        while (z < curr_end) {
            if (z == pivot) {
                z++;
                continue;
            }
            int cmp_1to2 = compareFunc(
                SORT_GETITEM(z),
                SORT_GETITEM(pivot)
            );
            if (cmp_1to2 < -1) {
                if (cmp_1to2 == S3D_SORT_ERR_UNSORTABLE) {
                    goto exitunsortable;
                }
                assert(cmp_1to2 == S3D_SORT_ERR_OOM);
                goto exitoom;
            }
            if ((cmp_1to2 == 1 && z < pivot) ||
                    (cmp_1to2 == -1 && z > pivot)) {
                // Need to switch.
                if (z <= pivot + 1) {
                    // Direct swap is enough: pivot can be pulled closer
                    // since anything above us is unsorted anyway.
                    memcpy(switchbuf, SORT_GETITEM(z),
                        itemsize);
                    memcpy(SORT_GETITEM(z),
                        SORT_GETITEM(pivot), itemsize);
                    memcpy(SORT_GETITEM(pivot),
                        switchbuf, itemsize);
                    pivot = z;
                } else {
                    // Stuff up to pivot and beyond is already
                    // sorted, so we can't just move pivot wildly, so
                    // instead pull this element towards the pivot:
                    // We swap pivot with neighbor in our direction,
                    // then swap z element (us) with that neighbor.
                    int64_t dir = (z - pivot);
                    assert(dir > 0);
                    assert(pivot + 1 < curr_end);
                    memcpy(switchbuf, SORT_GETITEM(pivot + 1),
                        itemsize);
                    memcpy(SORT_GETITEM(pivot + 1),
                        SORT_GETITEM(pivot), itemsize);
                    memcpy(SORT_GETITEM(pivot),
                        switchbuf, itemsize);
                    pivot += 1;
                    memcpy(switchbuf, SORT_GETITEM(pivot - 1),
                        itemsize);
                    memcpy(SORT_GETITEM(pivot - 1),
                        SORT_GETITEM(z), itemsize);
                    memcpy(SORT_GETITEM(z),
                        switchbuf, itemsize);
                }
            }
            z++;
        }
        // Post follow-up jobs for sub ranges around pivot:
        SORT_PUSHJOB(curr_start, pivot);
        SORT_PUSHJOB(pivot, curr_end);
        k++;
    }

    if (jobs_areonheap) {
        if (cache != NULL) {
            cache->cachedjobs = jobs;
            cache->cachedjobsalloc = jobs_alloc;
        } else {
            free(jobs);
        }
    }
    return 1;
}

#undef SORT_GETITEM
#undef SORT_PUSHJOB

#endif  // SPEW3D_IMPLEMENTATION

