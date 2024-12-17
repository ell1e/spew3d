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

#if defined(SPEW3D_IMPLEMENTATION) && \
    SPEW3D_IMPLEMENTATION != 0

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#else
#include <time.h>
#include <unistd.h>
#endif

S3DEXP uint64_t spew3d_time_Ticks() {
    #if defined(_WIN32) || defined(_WIN64)
    return GetTickCount64();
    #else
    struct timespec spec;
    clock_gettime(CLOCK_BOOTTIME, &spec);
    uint64_t ms1 = ((uint64_t)spec.tv_sec) * 1000ULL;
    uint64_t ms2 = ((uint64_t)spec.tv_nsec) / 1000000ULL;
    return ms1 + ms2;
    #endif
}

S3DEXP void spew3d_time_Sleep(uint64_t sleepms) {
    if (sleepms <= 0)
        return;
    #if defined(_WIN32) || defined(_WIN64)
    Sleep(sleepms);
    #else
    #if _POSIX_C_SOURCE >= 199309L
    struct timespec ts;
    ts.tv_sec = sleepms / 1000LL;
    ts.tv_nsec = (sleepms % 1000LL) * 1000000LL;
    nanosleep(&ts, NULL);
    #else
    if (sleepms >= 1000LL) {
        sleep(sleepms / 1000LL);
        sleepms = sleepms % 1000LL;
    }
    usleep(1000LL * sleepms);
    #endif
    #endif
}

#endif // SPEW3D_IMPLEMENTATION

