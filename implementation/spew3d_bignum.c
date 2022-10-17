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

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>


//#define _SPEW3D_BIGINT_DEBUG

S3DEXP int spew3d_bignum_VerifyStrInts(const char *v) {
    size_t vlen = strlen(v);
    if (vlen == 0)
        return 0;
    if (*v == '-') {
        if (vlen <= 1)
            return 0;
        v++;
        vlen--;
    }
    const char *vend = v + (uintptr_t)vlen;
    while (v != vend) {
        if (*v < '0' || *v > '9')
            return 0;
        v++;
    }
    return 1;
}

S3DEXP int spew3d_bignum_CompareStrInts(
        const char *v1, const char *v2
        ) {
    size_t v1len = strlen(v1);
    size_t v2len = strlen(v2);
    assert(spew3d_bignum_VerifyStrInts(v1) &&
           spew3d_bignum_VerifyStrInts(v2));

    printf("A: %s vs %s\n", v1, v2);

    if (v1len == 2 && strcmp(v1, "-0") == 0)
        return spew3d_bignum_CompareStrInts(v1 + 1, v2);
    if (v2len == 2 && strcmp(v2, "-0") == 0)
        return spew3d_bignum_CompareStrInts(v1, v2 + 1);

    if (v1[0] == '-') {
        if (v2[0] != '-') {
            if (v1[1] != '0' ||
                    spew3d_bignum_CompareStrInts(v1 + 1, "0") != 0)
                return -1;
            return spew3d_bignum_CompareStrInts("0", v2);
        }
        return -spew3d_bignum_CompareStrInts(v1 + 1, v2 + 1);
    } else if (v2[0] == '-') {
        if (v2[1] != '0' ||
                spew3d_bignum_CompareStrInts("0", v2 + 1) != 0)
            return 1;
        return spew3d_bignum_CompareStrInts(v1, "0");
    }
    assert(v1[0] != '-' && v2[0] != '-');

    while (v1len > 1 && *v1 == '0') {
        v1++;
        v1len--;
    }
    while (v2len > 1 && *v2 == '0') {
        v2++;
        v2len--;
    }

    printf("B %s %s\n", v1, v2);

    if (v1len > v2len)
        return 1;
    else if (v1len < v2len)
        return -1;

    const uint8_t *p1end = ((uint8_t*)v1) + (uintptr_t)v1len;
    const uint8_t *p1 = (uint8_t*)v1;
    const uint8_t *p2 = (uint8_t*)v2;
    while (p1 != p1end) {
        if ((*p1) > (*p2))
            return 1;
        else if ((*p1) < (*p2))
            return -1;
        p1++;
        p2++;
    }
    return 0;
}

#endif  // SPEW3D_IMPLEMENTATION

