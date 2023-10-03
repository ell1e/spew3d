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
#include <ctype.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#endif

S3DEXP int utf8_to_utf16(
        const uint8_t *input, int64_t input_len,
        uint16_t *outbuf, int64_t outbuflen,
        int64_t *out_len, int surrogateunescape,
        int surrogateescape
        ) {
    const uint8_t *p = input;
    uint64_t totallen = 0;
    int64_t i = 0;
    while (i < input_len) {
        if (outbuflen < 1)
            return 0;

        s3dcodepoint value = 0;
        int cpbyteslen = 0;
        int result = get_utf8_codepoint(
            p, input_len, &value, &cpbyteslen
        );
        if (!result) {
            if (surrogateescape) {
                value = 0xFFFDULL;
            } else {
                value = 0xD800ULL + value;
            }
            cpbyteslen = 1;
        } else if (value >= 0xD800ULL &&
                value < 0xE000ULL) {
            value = 0xFFFDULL;
            if (value <= 0xD800ULL + 255 &&
                    surrogateunescape) {
                s3dcodepoint value2 = 0;
                int cpbyteslen2 = 0;
                int result2 = get_utf8_codepoint(
                    p + cpbyteslen, input_len - cpbyteslen,
                    &value, &cpbyteslen2
                );
                if (result2 && value2 >= 0xD800ULL &&
                        value2 <= 0xD800ULL + 255) {
                    uint16_t origval;
                    uint8_t decoded1 = value - 0xD800ULL;
                    uint8_t decoded2 = value2 - 0xD800ULL;
                    memcpy(
                        &origval, &decoded1, sizeof(decoded1)
                    );
                    memcpy(
                        ((char*)&origval) + sizeof(decoded1),
                        &decoded2, sizeof(decoded2)
                    );
                    cpbyteslen += cpbyteslen2;
                    value = origval;
                }
            }
        }

        if ((int64_t)totallen + 1 > outbuflen)
            return 0;

        outbuf[totallen] = value;
        totallen++;
        p += cpbyteslen;
        i += cpbyteslen;
    }
    if (out_len) *out_len = (int64_t)totallen;
    return 1;
}

S3DEXP int utf16_to_utf8(
        const uint16_t *input, int64_t input_len,
        char *outbuf, int64_t outbuflen,
        int64_t *out_len, int surrogateescape
        ) {
    const uint16_t *p = input;
    uint64_t totallen = 0;
    int64_t i = 0;
    while (i < input_len) {
        if (outbuflen < 1)
            return 0;
        uint64_t value = *((uint16_t*)p);
        int is_valid_surrogate_pair = 0;
        if (value >= 0xD800 && value < 0xE000 &&
                i + 1 < input_len &&
                p[1] >= 0xD800 && p[1] < 0xE000) {
            // Possibly valid regular UTF-16 surrogates.
            uint64_t sg1 = value & 0x3FF;
            sg1 = sg1 << 10;
            uint64_t sg2 = p[1] & 0x3FF;
            uint64_t fullvalue = sg1 + sg2;
            if ((fullvalue < 0xD800 || fullvalue >= 0xE000) &&
                    fullvalue < 0x200000ULL) {
                value = fullvalue;
                is_valid_surrogate_pair = 1;
            }
        } else if (value >= 0xD800 && value < 0xE000) {
            if (surrogateescape) {
                // Special case: encode junk here with surrogates.
                // Write the 16-bit value out as two surrogate
                // escapes for each 8-bit part:
                uint8_t invalid_value[2];
                memcpy(invalid_value, &value, sizeof(uint16_t));
                value = 0xDC80ULL + invalid_value[0];
                int inneroutlen = 0;
                if (!write_codepoint_as_utf8(
                        (uint64_t)value, 0, 0,
                        outbuf, outbuflen, &inneroutlen
                        )) {
                    return 0;
                }
                assert(inneroutlen > 0);
                outbuflen -= inneroutlen;
                outbuf += inneroutlen;
                totallen += inneroutlen;
                value = 0xDC80ULL + invalid_value[1];
                inneroutlen = 0;
                if (!write_codepoint_as_utf8(
                        (uint64_t)value, 0, 0,
                        outbuf, outbuflen, &inneroutlen
                        )) {
                    return 0;
                }
                assert(inneroutlen > 0);
                outbuflen -= inneroutlen;
                outbuf += inneroutlen;
                totallen += inneroutlen;
                i++;
                p++;
                continue;
            } else {
                value = 0xFFFDULL;
            }
        }
        int inneroutlen = 0;
        if (!write_codepoint_as_utf8(
                (uint64_t)value, 0, 0,
                outbuf, outbuflen, &inneroutlen
                )) {
            return 0;
        }
        assert(inneroutlen > 0);
        p += (is_valid_surrogate_pair ? 2 : 1);
        outbuflen -= inneroutlen;
        outbuf += inneroutlen;
        totallen += inneroutlen;
        i += (is_valid_surrogate_pair ? 2 : 1);
    }
    if (out_len) *out_len = (int64_t)totallen;
    return 1;
}

S3DEXP size_t strlen16(const uint16_t *s) {
    const uint16_t *orig_s = s;
    while (*s != '\0')
        s++;
    return s - orig_s;
}

S3DEXP char *AS_U8_FROM_U16(const uint16_t *s) {
    #if (defined(_WIN32) || defined(_WIN64)) && \
        defined(USE_WINAPI_WIDECHAR)
    assert(sizeof(wchar_t) == sizeof(uint16_t));
    if (wcslen(s) == 0) {
        uint8_t *result = malloc(sizeof(*result));
        if (result) result[0] = '\0';
        return (char *)result;
    }
    int resultlen = WideCharToMultiByte(
        CP_UTF8, 0, s, (int)wcslen(s), NULL, 0,
        NULL, NULL
    );
    if (resultlen <= 0) {
        return NULL;
    }
    uint8_t *result = malloc((resultlen + 10) * sizeof(*result));
    int cvresult = WideCharToMultiByte(
        CP_UTF8, 0, s, (int)wcslen(s),
        (char *)result, resultlen + 10, NULL, NULL
    );
    if (cvresult <= 0 || cvresult > resultlen + 5) {
        free(result);
        return NULL;
    }
    result[cvresult] = '\0';
    return (char *)result;
    #else
    size_t slen = strlen16(s);
    char *s8 = malloc(slen * 4 + 1);
    if (!s8)
        return NULL;
    int64_t written = 0;
    if (!utf16_to_utf8(
            s, slen, s8, slen * 4 + 1,
            &written, 1
            )) {
        free(s8);
        return NULL;
    }
    return s8;
    #endif
}

S3DEXP uint16_t *AS_U16(const char *s) {
    #if (defined(_WIN32) || defined(_WIN64)) && \
        defined(USE_WINAPI_WIDECHAR)
    if (strlen(s) == 0) {
        uint16_t *result = malloc(sizeof(*result));
        if (result) result[0] = '\0';
        return result;
    }
    int resultlen = MultiByteToWideChar(
        CP_UTF8, 0, s, (int)strlen(s), NULL, 0
    );
    if (resultlen <= 0) {
        return NULL;
    }
    uint16_t *result = malloc((resultlen + 10) * sizeof(*result));
    int cvresult = MultiByteToWideChar(
        CP_UTF8, 0, s, (int)strlen(s),
        result, resultlen + 10
    );
    if (cvresult <= 0 || cvresult > resultlen + 5) {
        free(result);
        return NULL;
    }
    result[cvresult] = '\0';
    return result;
    #else
    size_t slen = strlen(s);
    uint16_t *s16 = malloc(slen * 2 + 2);
    if (!s16)
        return NULL;
    int64_t written = 0;
    if (!utf8_to_utf16(
            s, slen, s16, slen * 2 + 2,
            &written, 1, 0
            )) {
        free(s16);
        return NULL;
    }
    s16[written] = '\0';
    return s16;
    #endif
}

S3DEXP void utf8_char_to_lowercase(
        const char *s, int *out_origbyteslen,
        int *out_lowercasebyteslen,
        char *out_lowercased) {
    // FIXME: look it up in the unicode table.

    // Fallback implementation without unicode data:
    int l = utf8_char_len(s);
    *out_origbyteslen = l;
    if (l <= 1) {
        *out_lowercased = tolower(l);
        *out_lowercasebyteslen = 1;
    } else {
        memcpy(out_lowercased, s, l);
        *out_lowercasebyteslen = l;
    }
}

static int is_utf8_start(uint8_t c) {
    if ((int)(c & 0xE0) == (int)0xC0) {  // 110xxxxx
        return 1;
    } else if ((int)(c & 0xF0) == (int)0xE0) {  // 1110xxxx
        return 1;
    } else if ((int)(c & 0xF8) == (int)0xF0) {  // 11110xxx
        return 1;
    }
    return 0;
}

S3DEXP int utf8_char_len(const unsigned char *p) {
    if ((int)((*p) & 0xE0) == (int)0xC0)
        return 2;
    if ((int)((*p) & 0xF0) == (int)0xE0)
        return 3;
    if ((int)((*p) & 0xF8) == (int)0xF0)
        return 4;
    return 1;
}

S3DEXP int get_utf8_codepoint(
        const unsigned char *p, int size,
        s3dcodepoint *out, int *cpbyteslen
        ) {
    if (size < 1)
        return 0;
    if (!is_utf8_start(*p)) {
        if (*p > 127)
            return 0;
        if (out) *out = (uint64_t)(*p);
        if (cpbyteslen) *cpbyteslen = 1;
        return 1;
    }
    uint8_t c = (*(uint8_t*)p);
    if ((int)(c & 0xE0) == (int)0xC0 && size >= 2) {  // p[0] == 110xxxxx
        if ((int)(*(p + 1) & 0xC0) != (int)0x80) { // p[1] != 10xxxxxx
            return 0;
        }
        if (size >= 3 &&
                (int)(*(p + 2) & 0xC0) == (int)0x80) { // p[2] == 10xxxxxx
            return 0;
        }
        uint64_t c = (   // 00011111 of first byte
            (uint64_t)(*p) & (uint64_t)0x1FULL
        ) << (uint64_t)6ULL;
        c += (  // 00111111 of second byte
            (uint64_t)(*(p + 1)) & (uint64_t)0x3FULL
        );
        if (c <= 127ULL)
            return 0;  // not valid to be encoded with two bytes.
        if (out) *out = c;
        if (cpbyteslen) *cpbyteslen = 2;
        return 1;
    }
    if ((int)(c & 0xF0) == (int)0xE0 && size >= 3) {  // p[0] == 1110xxxx
        if ((int)(*(p + 1) & 0xC0) != (int)0x80) { // p[1] != 10xxxxxx
            return 0;
        }
        if ((int)(*(p + 2) & 0xC0) != (int)0x80) { // p[2] != 10xxxxxx
            return 0;
        }
        if (size >= 4 &&
                (int)(*(p + 3) & 0xC0) == (int)0x80) { // p[3] == 10xxxxxx
            return 0;
        }
        uint64_t c = (   // 00001111 of first byte
            (uint64_t)(*p) & (uint64_t)0xFULL
        ) << (uint64_t)12ULL;
        c += (  // 00111111 of second byte
            (uint64_t)(*(p + 1)) & (uint64_t)0x3FULL
        ) << (uint64_t)6ULL;
        c += (  // 00111111 of third byte
            (uint64_t)(*(p + 2)) & (uint64_t)0x3FULL
        );
        if (c <= 0x7FFULL)
            return 0;  // not valid to be encoded with three bytes.
        if (c >= 0xD800ULL && c <= 0xDFFFULL) {
            // UTF-16 surrogate code points may not be used in UTF-8
            // (in part because we re-use them to store invalid bytes)
            return 0;
        }
        if (out) *out = c;
        if (cpbyteslen) *cpbyteslen = 3;
        return 1;
    }
    if ((int)(c & 0xF8) == (int)0xF0 && size >= 4) {  // p[0] == 11110xxx
        if ((int)(*(p + 1) & 0xC0) != (int)0x80) { // p[1] != 10xxxxxx
            return 0;
        }
        if ((int)(*(p + 2) & 0xC0) != (int)0x80) { // p[2] != 10xxxxxx
            return 0;
        }
        if ((int)(*(p + 3) & 0xC0) != (int)0x80) { // p[3] != 10xxxxxx
            return 0;
        }
        if (size >= 5 &&
                (int)(*(p + 4) & 0xC0) == (int)0x80) { // p[4] == 10xxxxxx
            return 0;
        }
        uint64_t c = (   // 00000111 of first byte
            (uint64_t)(*p) & (uint64_t)0x7ULL
        ) << (uint64_t)18ULL;
        c += (  // 00111111 of second byte
            (uint64_t)(*(p + 1)) & (uint64_t)0x3FULL
        ) << (uint64_t)12ULL;
        c += (  // 00111111 of third byte
            (uint64_t)(*(p + 2)) & (uint64_t)0x3FULL
        ) << (uint64_t)6ULL;
        c += (  // 00111111 of fourth byte
            (uint64_t)(*(p + 3)) & (uint64_t)0x3FULL
        );
        if (c <= 0xFFFFULL)
            return 0;  // not valid to be encoded with four bytes.
        if (out) *out = c;
        if (cpbyteslen) *cpbyteslen = 4;
        return 1;
    }
    return 0;
}

S3DEXP int starts_with_valid_utf8_char(
        const unsigned char *p, int size
        ) {
    if (!get_utf8_codepoint(p, size, NULL, NULL))
        return 0;
    return 1;
}

S3DEXP int write_codepoint_as_utf8(
        s3dcodepoint codepoint, int surrogateunescape,
        int questionmarkescape,
        char *out, int outbuflen, int *outlen
        ) {
    if (surrogateunescape &&
            codepoint >= 0xDC80ULL + 0 &&
            codepoint <= 0xDC80ULL + 255) {
        if (outbuflen < 1) return 0;
        ((uint8_t *)out)[0] = (int)(codepoint - 0xDC80ULL);
        if (outbuflen >= 2)
            out[1] = '\0';
        if (outlen) *outlen = 1;
        return 1;
    }
    if (codepoint < 0x80ULL) {
        if (outbuflen < 1) return 0;
        ((uint8_t *)out)[0] = (int)codepoint;
        if (outbuflen >= 2)
            out[1] = '\0';
        if (outlen) *outlen = 1;
        return 1;
    } else if (codepoint < 0x800ULL) {
        uint64_t byte2val = (codepoint & 0x3FULL);
        uint64_t byte1val = (codepoint & 0x7C0ULL) >> 6;
        if (outbuflen < 2) return 0;
        ((uint8_t *)out)[0] = (int)(byte1val | 0xC0);
        ((uint8_t *)out)[1] = (int)(byte2val | 0x80);
        if (outbuflen >= 3)
            out[2] = '\0';
        if (outlen) *outlen = 2;
        return 1;
    } else if (codepoint < 0x10000ULL) {
        uint64_t byte3val = (codepoint & 0x3FULL);
        uint64_t byte2val = (codepoint & 0xFC0ULL) >> 6;
        uint64_t byte1val = (codepoint & 0xF000ULL) >> 12;
        if (outbuflen < 3) return 0;
        ((uint8_t *)out)[0] = (int)(byte1val | 0xE0);
        ((uint8_t *)out)[1] = (int)(byte2val | 0x80);
        ((uint8_t *)out)[2] = (int)(byte3val | 0x80);
        if (outbuflen >= 4)
            out[3] = '\0';
        if (outlen) *outlen = 3;
        return 1;
    } else if (codepoint < 0x200000ULL) {
        uint64_t byte4val = (codepoint & 0x3FULL);
        uint64_t byte3val = (codepoint & 0xFC0ULL) >> 6;
        uint64_t byte2val = (codepoint & 0x3F000ULL) >> 12;
        uint64_t byte1val = (codepoint & 0x1C0000ULL) >> 18;
        if (outbuflen < 4) return 0;
        ((uint8_t *)out)[0] = (int)(byte1val | 0xF0);
        ((uint8_t *)out)[1] = (int)(byte2val | 0x80);
        ((uint8_t *)out)[2] = (int)(byte3val | 0x80);
        ((uint8_t *)out)[3] = (int)(byte4val | 0x80);
        if (outbuflen >= 5)
            out[4] = '\0';
        if (outlen) *outlen = 4;
        return 1;
    } else if (questionmarkescape) {
        if (outbuflen < 3) return 0;
        ((uint8_t *)out)[0] = (int)0xEF;
        ((uint8_t *)out)[1] = (int)0xBF;
        ((uint8_t *)out)[2] = (int)0xBD;
        if (outbuflen >= 4)
            out[3] = '\0';
        if (outlen) *outlen = 3;
        return 1;
    } else {
        return 0;
    }
}

#endif  // SPEW3D_IMPLEMENTATION

