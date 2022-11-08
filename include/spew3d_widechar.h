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

#ifndef SPEW3D_WIDECHAR_H_
#define SPEW3D_WIDECHAR_H_

#include <stdint.h>

typedef uint64_t s3dcodepoint;

int starts_with_valid_utf8_char(
    const unsigned char *p, int size
);

int utf8_char_len(const unsigned char *p);

int get_utf8_codepoint(
    const unsigned char *p, int size,
    s3dcodepoint *out, int *cpbyteslen
);

char *AS_U8_FROM_U16(const uint16_t *s);

uint16_t *AS_U16(const char *s);

size_t strlen16(const uint16_t *s);

int write_codepoint_as_utf8(
    s3dcodepoint codepoint, int surrogateunescape,
    int questionmarkescape,
    char *out, int outbuflen, int *outlen
);

int utf8_to_utf16(
    const uint8_t *input, int64_t input_len,
    uint16_t *outbuf, int64_t outbuflen,
    int64_t *out_len, int surrogateunescape,
    int surrogateescape
);

int utf16_to_utf8(
    const uint16_t *input, int64_t input_len,
    char *outbuf, int64_t outbuflen,
    int64_t *out_len, int surrogateescape
);

#endif  // SPEW3D_WIDECHAR_H_
