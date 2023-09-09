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
#include <stdio.h>
#include <string.h>
#include <unistd.h>


//#define _SPEW3D_BIGINT_DEBUG


S3DEXP void spew3d_bignum_PrintFloatBuf(
        const char *v, size_t vlen
        ) {
    const char *vend = v + vlen;
    while (v != vend) {
        printf("%c", *((unsigned char *)v));
        v++;
    }
}

S3DEXP int spew3d_bignum_VerifyStrFloatBuf(
        const char *v, size_t vlen
        ) {
    if (vlen == 0)
        return 0;
    if (*v == '-') {
        if (vlen <= 1)
            return 0;
        v++;
        vlen--;
    }

    int haddot = 0;
    const char *vstart = v;
    const char *vend = v + (uintptr_t)vlen;
    while (v != vend) {
        if ((*v < '0' || *v > '9') && *v != '.')
            return 0;
        if (*v == '.') {
            if (v == vstart || haddot || (v + 1 == vend))
                return 0;
            haddot = 1;
        }
        v++;
    }
    return 1;
}

S3DEXP int spew3d_bignum_VerifyStrFloat(
        const char *v) {
    return spew3d_bignum_VerifyStrFloatBuf(
        v, strlen(v)
    );
}

S3DEXP int spew3d_bignum_VerifyStrIntBuf(
        const char *v, size_t vlen
        ) {
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

S3DEXP int spew3d_bignum_VerifyStrInt(
        const char *v) {
    return spew3d_bignum_VerifyStrIntBuf(v, strlen(v));
}

S3DEXP int spew3d_bignum_CompareStrInts(
        const char *v1, const char *v2
        ) {
    return spew3d_bignum_CompareStrIntsBuf(
        v1, strlen(v1), v2, strlen(v2)
    );
}

S3DEXP int spew3d_bignum_CompareStrIntsBuf(
        const char *v1, size_t v1len,
        const char *v2, size_t v2len
        ) {
    assert(spew3d_bignum_VerifyStrIntBuf(v1, v1len) &&
           spew3d_bignum_VerifyStrIntBuf(v2, v2len));

    if (v1len == 2 && memcmp(v1, "-0", 2) == 0)
        return spew3d_bignum_CompareStrIntsBuf(
            v1 + 1, v1len - 1, v2, v2len);
    if (v2len == 2 && memcmp(v2, "-0", 2) == 0)
        return spew3d_bignum_CompareStrIntsBuf(
            v1, v1len, v2 + 1, v2len - 1);

    if (v1[0] == '-') {
        if (v2[0] != '-') {
            if (v1[1] != '0' ||
                    spew3d_bignum_CompareStrIntsBuf(
                        v1 + 1, v1len - 1, "0", 1) != 0
                    )
                return -1;
            return spew3d_bignum_CompareStrIntsBuf(
                "0", 1, v2, v2len
            );
        }
        return -spew3d_bignum_CompareStrIntsBuf(
            v1 + 1, v1len - 1, v2 + 1, v2len - 1
        );
    } else if (v2[0] == '-') {
        if (v2[1] != '0' ||
                spew3d_bignum_CompareStrIntsBuf(
                    "0", 1, v2 + 1, v2len - 1
                ) != 0)
            return 1;
        return spew3d_bignum_CompareStrIntsBuf(
            v1, v1len, "0", 1
        );
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

S3DEXP int spew3d_bignum_CompareStrFloats(
        const char *v1, const char *v2
        ) {
    return spew3d_bignum_CompareStrFloatsBuf(
        v1, strlen(v1), v2, strlen(v2)
    );
}

S3DEXP int spew3d_bignum_CompareStrFloatsBuf(
        const char *v1, size_t v1len,
        const char *v2, size_t v2len
        ) {
    assert(spew3d_bignum_VerifyStrFloatBuf(v1, v1len) &&
           spew3d_bignum_VerifyStrFloatBuf(v2, v2len));

    size_t dot1 = v1len;
    size_t dot2 = v2len;
    const int iend = ((v1len > v2len) ? v1len : v2len);
    size_t i = 1;
    while (i < iend) {
        if (S3DUNLIKELY(i < v1len && v1[i] == '.')) {
            assert(dot1 == v1len);
            dot1 = i;
        }
        if (S3DUNLIKELY(i < v2len && v2[i] == '.')) {
            assert(dot2 == v2len);
            dot2 = i;
        }
        i++;
    }

    int result = spew3d_bignum_CompareStrIntsBuf(
        v1, dot1, v2, dot2
    );
    if (S3DLIKELY(result != 0 ||
            (dot1 == v1len && dot2 == v2len)))
        return result;

    int v1neg = (v1[0] == '-');
    int v2neg = (v2[0] == '-');

    const char *zerobuf = "0";
    size_t zerobuflen = 1;
    if (S3DUNLIKELY(dot1 == v1len)) {
        assert(dot2 < v2len && dot2 >= 1);
        v1 = zerobuf;
        v1len = zerobuflen;
        v2 += dot2 + 1;
        v2len -= dot2 + 1;
    } else if (S3DUNLIKELY(dot2 == v2len)) {
        assert(dot1 < v1len && dot1 >= 1);
        v2 = zerobuf;
        v2len = zerobuflen;
        v1 += dot1 + 1;
        v1len -= dot1 + 1;
    } else {
        v2 += dot2 + 1;
        v2len -= dot2 + 1;
        v1 += dot1 + 1;
        v1len -= dot1 + 1;
    }

    while (S3DUNLIKELY(v1len > 1 && v1[v1len - 1] == '0'))
        v1len--;
    while (S3DUNLIKELY(v2len > 1 && v2[v2len - 1] == '0'))
        v2len--;
    size_t minlen = ((v1len < v2len) ? v1len : v2len);

    result = spew3d_bignum_CompareStrIntsBuf(
        v1, minlen, v2, minlen
    );
    if (result == 0) {
        if (v1len > v2len) {
            result = 1;
        } else if (v2len > v1len) {
            result = -1;
        } else {
            return 0;
        }
    }
    if (!v1neg && !v2neg)
        return result;
    if (v1neg && v2neg)
        return -result;
    if (v1neg)
        return -1;
    else
        return 1;
}

S3DHID char *_internal_spew3d_bignum_AddPosNonfracStrFloatsBuf(
        const char *v1, size_t v1len, size_t v1imaginaryzeroes,
        const char *v2, size_t v2len, size_t v2imaginaryzeroes,
        int with_initial_carryover,
        char *use_buf,
        uint64_t *out_len,
        int *out_endedwithcarryover
        ) {
    assert(spew3d_bignum_VerifyStrFloatBuf(v1, v1len) &&
           spew3d_bignum_VerifyStrFloatBuf(v2, v2len));
    char *result = use_buf;
    if (!result) {
        result = malloc(
            (v1len + v1imaginaryzeroes >
            v2len + v2imaginaryzeroes) ?
            (v1len + v1imaginaryzeroes + 2) :
            (v2len + v2imaginaryzeroes + 2)
        );
        if (!result)
            return NULL;
    }
    char *write = result;
    int carryover = (with_initial_carryover != 0);
    int resultlen = 0;
    const char *read1 = v1 + v1len - 1;
    const char *read2 = v2 + v2len - 1;
    const char *last1 = v1;
    const char *last2 = v2;
    while (S3DLIKELY(read1 != last1 && read2 != last2)) {
        int skippeddigit1 = 0;
        int digit1 = 0;
        if (S3DUNLIKELY(v1imaginaryzeroes > 0)) {
            skippeddigit1 = 1;
            v1imaginaryzeroes--;
        } else {
            digit1 = (*read1) - '0';
        }
        int skippeddigit2 = 0;
        int digit2 = 0;
        if (S3DUNLIKELY(v2imaginaryzeroes > 0)) {
            skippeddigit2 = 1;
            v2imaginaryzeroes--;
        } else {
            digit2 = (*read2) - '0';
        }
        int resultdigit = (digit1 + digit2 + carryover);
        carryover = 0;
        if (S3DUNLIKELY(resultdigit > 9)) {
            assert(resultdigit < 20);
            carryover = 1;
            resultdigit -= 10;
        }
        *write = resultdigit + '0';
        write++;
        if (S3DLIKELY(!skippeddigit1))
            read1--;
        if (S3DLIKELY(!skippeddigit2))
            read2--;
    }
    while (1) {
        int skippeddigit1 = 0;
        int digit1 = 0;
        if (S3DUNLIKELY(v1imaginaryzeroes > 0)) {
            skippeddigit1 = 1;
            v1imaginaryzeroes--;
        } else {
            if (S3DLIKELY(read1))
                digit1 = (*read1) - '0';
        }
        int skippeddigit2 = 0;
        int digit2 = 0;
        if (S3DUNLIKELY(v2imaginaryzeroes > 0)) {
            skippeddigit2 = 1;
            v2imaginaryzeroes--;
        } else {
            if (S3DLIKELY(read2))
                digit2 = (*read2) - '0';
        }
        assert(digit1 >= 0 && digit1 <= 9);
        assert(digit2 >= 0 && digit2 <= 9);
        int resultdigit = (digit1 + digit2 + carryover);
        carryover = 0;
        if (S3DUNLIKELY(resultdigit > 9)) {
            assert(resultdigit <= 19);
            carryover = 1;
            resultdigit -= 10;
        }
        *write = resultdigit + '0';
        write++;
        if (S3DLIKELY(read1)) {
            if (S3DUNLIKELY(read1 == last1 && !skippeddigit1)) {
                read1 = NULL;
                if (!read2 || (read2 == last2 && !skippeddigit2)) {
                    if (carryover) {
                        *write = '1';
                        write++;
                        if (out_endedwithcarryover)
                            *out_endedwithcarryover = 1;
                    } else {
                        if (out_endedwithcarryover)
                            *out_endedwithcarryover = 0;
                    }
                    *write = '\0';
                    break;
                }
            }
        }
        if (S3DLIKELY(read2)) {
            if (S3DUNLIKELY(read2 == last2 && !skippeddigit2)) {
                read2 = NULL;
                if (!read1 || (read1 == last1 && !skippeddigit1)) {
                    if (carryover) {
                        *write = '1';
                        write++;
                        if (out_endedwithcarryover)
                            *out_endedwithcarryover = 1;
                    } else {
                        if (out_endedwithcarryover)
                            *out_endedwithcarryover = 0;
                    }
                    *write = '\0';
                    break;
                }
            }
        }
        if (S3DLIKELY(read1 && !skippeddigit1))
            read1--;
        if (S3DLIKELY(read2 && !skippeddigit2))
            read2--;
    }
    uint64_t result_len = (write - result);
    spew3d_stringutil_ReverseBufBytes(result, result_len);
    *out_len = result_len;
    return result;
}

static int _compare_plain_digit_nos(
        const char *v1, size_t v1len, size_t v1imaginaryzeroes,
        const char *v2, size_t v2len, size_t v2imaginaryzeroes
        ) {
    assert(v1 != NULL && v2 != NULL);
    if (v1len + v1imaginaryzeroes > v2len + v2imaginaryzeroes)
        return 1;
    if (v2len + v2imaginaryzeroes > v1len + v1imaginaryzeroes)
        return -1;
    const char *v1c = v1 + v1len - 1;
    const char *v2c = v2 + v2len - 1;
    const char *v1end = v1;
    const char *v2end = v2;
    if (S3DUNLIKELY(v1imaginaryzeroes > 0 && v2imaginaryzeroes > 0)) {
        while (S3DLIKELY(v1imaginaryzeroes > 0 &&
                v2imaginaryzeroes > 0)) {
            v1imaginaryzeroes--;
            v2imaginaryzeroes--;
        }
    }
    while (1) {
        assert(v1c >= v1end && v2c >= v2end);
        int skippeddigit1 = 0;
        int skippeddigit2 = 0;
        int cmp = 0;
        if (S3DUNLIKELY(v1imaginaryzeroes > 0)) {
            cmp = 0 - ((int)*((uint8_t *)v2c));
            skippeddigit1 = 1;
            v1imaginaryzeroes--;
        } else if (S3DUNLIKELY(v2imaginaryzeroes > 0)) {
            cmp = ((int)*((uint8_t *)v1c));
            skippeddigit2 = 1;
            v2imaginaryzeroes--;
        } else {
            cmp = (((int)*((uint8_t *)v1c)) -
                ((int)*((uint8_t *)v2c)));
        }
        if (S3DUNLIKELY(v1c == v1end && !skippeddigit1)) {
            if (v2c == v2end && !skippeddigit2)
                return cmp;
            return -1;
        } else if (S3DUNLIKELY(v2c == v2end && !skippeddigit2)) {
            if (v1c == v1end && !skippeddigit1)
                return cmp;
            return 1;
        }
        if (!skippeddigit1)
            v1c--;
        if (!skippeddigit2)
            v2c--;
    }
}

S3DHID char *_internal_spew3d_bignum_SubPosNonfracStrFloatsBuf(
        const char *v1, size_t v1len, size_t v1imaginaryzeroes,
        const char *v2, size_t v2len, size_t v2imaginaryzeroes,
        int ignoredots,
        int with_initial_carryover,
        char *use_buf,
        uint64_t *out_len
        ) {
    assert(spew3d_bignum_VerifyStrFloatBuf(v1, v1len) &&
           spew3d_bignum_VerifyStrFloatBuf(v2, v2len));
    if (_compare_plain_digit_nos(v1, v1len,
            v1imaginaryzeroes, v2, v2len,
            v2imaginaryzeroes) < 0) {
        assert(with_initial_carryover >= 0);
        // Special case: result will be negative since substracted
        // number is larger. The code below doesn't support that, so
        // we need to switch them out:
        char *result = use_buf;
        if (!result) {
            // Same calculation as a few lines below, but extra
            // byte for minus digit:
            result = malloc(v2len + 4);
            if (!result)
                return NULL;
        }
        char *inner_result = (
            _internal_spew3d_bignum_SubPosNonfracStrFloatsBuf(
                v2, v2len, v2imaginaryzeroes, v1, v1len,
                v1imaginaryzeroes, ignoredots,
                (with_initial_carryover > 0 ? -1 : 0),
                result + 1, out_len
            ));
        result[0] = '-';
        (*out_len)++;
        return result;
    }
    assert(v1imaginaryzeroes != 0 || v2imaginaryzeroes != 0 ||
        spew3d_bignum_CompareStrFloatsBuf(
            v1, v1len, v2, v2len
        ) >= 0);
    char *result = use_buf;
    if (!result) {
        // A carry over can add a digit, then the result might be negative
        // which adds one byte, and then we want a null terminator. This
        // Sums up to worst case 3 bytes extra length:
        result = malloc(
            (v1len + v1imaginaryzeroes >
            v2len + v2imaginaryzeroes) ?
            (v1len + v1imaginaryzeroes + 3) :
            (v2len + v2imaginaryzeroes + 3)
        );
        if (!result)
            return NULL;
    }
    char *write = result;
    int carryover = with_initial_carryover;
    int resultlen = 0;
    const char *read1 = v1 + v1len - 1;
    const char *read2 = v2 + v2len - 1;
    const char *last1 = v1;
    const char *last2 = v2;
    while (S3DLIKELY(read1 != last1 && read2 != last2)) {
        if (S3DUNLIKELY(ignoredots))
            if (S3DUNLIKELY((*read1) == '.')) {
                read1--;
                continue;
            }
            if (S3DUNLIKELY((*read2) == '.')) {
                read2--;
                continue;
            }
        int skippeddigit1 = 0;
        int digit1 = 0;
        if (S3DUNLIKELY(v1imaginaryzeroes > 0)) {
            skippeddigit1 = 1;
            v1imaginaryzeroes--;
        } else {
            digit1 = (*read1) - '0';
        }
        int skippeddigit2 = 0;
        int digit2 = 0;
        if (S3DUNLIKELY(v2imaginaryzeroes > 0)) {
            skippeddigit2 = 1;
            v2imaginaryzeroes--;
        } else {
            digit2 = (*read2) - '0';
        }
        assert(digit1 >= 0 && digit1 <= 9);
        assert(digit2 >= 0 && digit2 <= 9);
        int resultdigit = (digit1 - digit2 - carryover);
        carryover = 0;
        if (S3DUNLIKELY(resultdigit < 0)) {
            assert(resultdigit >= -10);
            carryover = 1;
            resultdigit = (10 + resultdigit);
        } else if (S3DUNLIKELY(resultdigit >= 10)) {
            // This can happen when called with v1 and v2
            // reversed.
            assert(resultdigit < 20);
            carryover = -1;
            resultdigit -= 10;
        }
        *write = resultdigit + '0';
        write++;
        if (!skippeddigit1)
            read1--;
        if (!skippeddigit2)
            read2--;
    }
    while (1) {
        if (S3DUNLIKELY(ignoredots))
            if (S3DUNLIKELY(read1 != NULL &&
                    (*read1) == '.')) {
                read1--;
                continue;
            }
            if (S3DUNLIKELY(read2 != NULL &&
                    (*read2) == '.')) {
                read2--;
                continue;
            }
        int skippeddigit1 = 0;
        int digit1 = 0;
        if (S3DUNLIKELY(v1imaginaryzeroes > 0)) {
            skippeddigit1 = 1;
            v1imaginaryzeroes--;
        } else {
            if (S3DLIKELY(read1))
                digit1 = (*read1) - '0';
        }
        int skippeddigit2 = 0;
        int digit2 = 0;
        if (S3DUNLIKELY(v2imaginaryzeroes > 0)) {
            skippeddigit2 = 1;
            v2imaginaryzeroes--;
        } else {
            if (S3DLIKELY(read2))
                digit2 = (*read2) - '0';
        }
        assert(digit1 >= 0 && digit1 <= 9);
        assert(digit2 >= 0 && digit2 <= 9);
        int resultdigit = (digit1 - digit2 - carryover);
        carryover = 0;
        if (S3DUNLIKELY(resultdigit < 0)) {
            assert(resultdigit >= -10);
            carryover = 1;
            resultdigit = (10 + resultdigit);
        } else if (S3DUNLIKELY(resultdigit >= 10)) {
            // This can happen when called with v1 and v2
            // reversed.
            assert(resultdigit < 20);
            carryover = -1;
            resultdigit -= 10;
        }
        *write = resultdigit + '0';
        write++;
        if (S3DLIKELY(read1)) {
            if (S3DUNLIKELY(read1 == last1 && !skippeddigit1)) {
                read1 = NULL;
                if (!read2 || (read2 == last2 && !skippeddigit2)) {
                    if (carryover) {
                        *write = '1';
                        write++;
                    }
                    *write = '\0';
                    break;
                }
            }
        }
        if (S3DLIKELY(read2)) {
            if (S3DUNLIKELY(read2 == last2 && !skippeddigit2)) {
                read2 = NULL;
                if (!read1 || (read1 == last1 && !skippeddigit1)) {
                    if (carryover) {
                        *write = '1';
                        write++;
                    }
                    *write = '\0';
                    break;
                }
            }
        }
        if (S3DLIKELY(read1 && !skippeddigit1))
            read1--;
        if (S3DLIKELY(read2 && !skippeddigit2))
            read2--;
    }
    uint64_t result_len = (write - result);
    if (S3DUNLIKELY(result > 0 &&
            result[result_len - 1] == '0')) {
        while (1) {
            result_len--;
            if (S3DUNLIKELY(result_len == 0 ||
                    result[result_len - 1] != '0'))
                break;
        }
    }
    result[result_len] = '\0';
    spew3d_stringutil_ReverseBufBytes(result, result_len);
    *out_len = result_len;
    return result;
}

S3DEXP char *spew3d_bignum_AddStrFloatBufsEx(
        const char *v1, size_t v1len,
        const char *v2, size_t v2len,
        char *use_buf, int truncate_fractional,
        uint64_t *out_len
        ) {
    assert(spew3d_bignum_VerifyStrFloatBuf(v1, v1len) &&
           spew3d_bignum_VerifyStrFloatBuf(v2, v2len));
    char *resultbuf = use_buf;
    int worstcaselen = (
        // For non-fractional numbers:
        // Addition or substraction can produce a max additional
        // digit before AND after the fractional dot. On top, a new
        // leading minus might appear. With null terminator, that's
        // four additional bytes total in the worst case, +4 length.
        // For fractional:
        // It's above but not with the longest number, but rather
        // the longest non-fractional half of both and the longest
        // fractional half of both. We approximate this as
        // both lengths and above +4 added.
        v1len + v2len + 4
    );
    int freeresultbufonerror = 0;
    if (!resultbuf) {
        resultbuf = malloc(worstcaselen);
        if (!resultbuf)
            return NULL;
        freeresultbufonerror = 1;
    }
    int positiveonly = 0;
    if (S3DUNLIKELY(v1[0] == '-' && v2[0] == '-')) {
        char *innerresult = spew3d_bignum_AddStrFloatBufsEx(
            v1 + 1, v1len - 1, v2 + 1, v2len - 1,
            resultbuf + 1, truncate_fractional,
            out_len
        );
        assert(innerresult != NULL);
        (*out_len)++;
        resultbuf[0] = '-';
        return resultbuf;
    } else if (S3DLIKELY(v1[0] == '-' && v2[0] != '-')) {
        // Reverse, so the 2nd number is always the substracted one:
        return spew3d_bignum_AddStrFloatBufsEx(
            v2, v2len, v1, v1len,
            resultbuf, truncate_fractional,
            out_len
        );
    }
    assert(v1[0] != '-');
    int dot1pos = v1len;
    int dot2pos = v2len;
    int i = 0;
    while (S3DLIKELY(i < v1len)) {
        if (S3DUNLIKELY(v1[i] == '.')) {
            dot1pos = i;
            break;
        }
        i++;
    }
    i = 0;
    while (S3DLIKELY(i < v2len)) {
        if (S3DUNLIKELY(v2[i] == '.')) {
            dot2pos = i;
            break;
        }
        i++;
    }
    if (S3DLIKELY(dot1pos == v1len && dot2pos == v2len)) {
        if (S3DLIKELY(v2[0] != '-')) {
            return _internal_spew3d_bignum_AddPosNonfracStrFloatsBuf(
                v1, v1len, 0, v2, v2len, 0,
                0, resultbuf, out_len, NULL
            );
        } else {
            return _internal_spew3d_bignum_SubPosNonfracStrFloatsBuf(
                v1, v1len, 0, v2 + 1, v2len - 1, 0,
                0, 0, resultbuf, out_len
            );
        }
    }
    if (S3DLIKELY(v2[0] != '-')) {
        // Easier case, both numbers are positive.
        // In this case, go the more efficient route of handling the
        // fractional part separately without moving things around:
        char _bufstack[256];
        char *fracbuf = _bufstack;
        int heapfracbuf = 0;
        if (truncate_fractional) {
            if (v1len - dot1pos > S3D_BIGNUM_MAXFRACTIONDIGITS)
                v1len -= (v1len - dot1pos) -
                    S3D_BIGNUM_MAXFRACTIONDIGITS;
            if (v2len - dot2pos > S3D_BIGNUM_MAXFRACTIONDIGITS)
                v2len -= (v2len - dot2pos) -
                    S3D_BIGNUM_MAXFRACTIONDIGITS;
        }
        if ((v1len - dot1pos) > 246 ||
                (v2len - dot2pos) > 246) {
            heapfracbuf = 1;
            fracbuf = NULL;
        }
        int frac1zeropad = (
            ((v1len - dot1pos) < (v2len - dot2pos)) ?
            ((v2len - dot2pos) - (v1len - dot1pos)) : 0
        );
        int frac2zeropad = (
            ((v2len - dot2pos) < (v1len - dot1pos)) ?
            ((v1len - dot1pos) - (v2len - dot2pos)) : 0
        );
        char zeronum[] = "0";
        int zeronumlen = 1;
        char *fractioninnerresult = NULL;
        const char *frac1 = ((v1len - dot1pos > 1) ?
            (v1 + dot1pos + 1) : zeronum);
        int frac1len = ((v1len - dot1pos > 1) ?
            (v1len - dot1pos - 1) : zeronumlen);
        const char *frac2 = ((v2len - dot2pos > 1) ?
            (v2 + dot2pos + 1) : zeronum);
        int frac2len = ((v2len - dot2pos > 1) ?
            (v2len - dot2pos - 1) : zeronumlen);
        int fracendswithcarry = 0;
        uint64_t fracoutlen = 0;
        fractioninnerresult = (
            _internal_spew3d_bignum_AddPosNonfracStrFloatsBuf(
                frac1, frac1len, frac1zeropad,
                frac2, frac2len, frac2zeropad, 0,
                fracbuf, &fracoutlen, &fracendswithcarry
            ));
        if (heapfracbuf && fractioninnerresult == NULL) {
            if (freeresultbufonerror)
                free(resultbuf);
            return NULL;
        }
        assert(fractioninnerresult != NULL);
        uint64_t nonfracoutlen = 0;
        char *nonfracresult = (
            _internal_spew3d_bignum_AddPosNonfracStrFloatsBuf(
                v1, dot1pos, 0, v2, dot2pos, 0, fracendswithcarry,
                resultbuf, &nonfracoutlen, NULL
            ));
        assert(nonfracresult != NULL);
        resultbuf[nonfracoutlen] = '.';
        memcpy(resultbuf + nonfracoutlen + 1,
            fractioninnerresult + fracendswithcarry,
            fracoutlen - fracendswithcarry);
        resultbuf[nonfracoutlen + 1 + fracoutlen - fracendswithcarry] = '\0';
        *out_len = nonfracoutlen + 1 + fracoutlen - fracendswithcarry;
        if (heapfracbuf)
            free(fractioninnerresult);
        return resultbuf;
    }
    assert(0);
}

S3DEXP char *spew3d_bignum_AddStrFloatBufs(
        const char *v1, size_t v1len,
        const char *v2, size_t v2len,
        uint64_t *out_len
        ) {
    return spew3d_bignum_AddStrFloatBufsEx(
        v1, v1len, v2, v2len, NULL, 1, out_len
    );
}

#endif  // SPEW3D_IMPLEMENTATION

