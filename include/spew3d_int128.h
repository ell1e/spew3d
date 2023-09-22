/* Copyright (c) 2023, ellie/@ell1e & Spew3D Team (see AUTHORS.md).

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

#ifndef SPEW3D_INT128_H_
#define SPEW3D_INT128_H_

#include <stdint.h>

typedef struct s3dint128_t_struct {
    uint64_t highsignificance;
    int64_t lowsignificance;
} s3dint128_t_struct;

static inline s3dint128_t_struct spew3d_int128_mult(
        s3dint128_t_struct x, s3dint128_t_struct y
        ) {
    s3dint128_t_struct result;
    r128Mul((R128*)&result, ((R128*)&x), ((R128*)&y));
    return result;
}

static inline s3dint128_t_struct spew3d_int128_div(
        s3dint128_t_struct x, s3dint128_t_struct y
        ) {
    s3dint128_t_struct result;
    r128Div((R128*)&result, ((R128*)&x), ((R128*)&y));
    return result;
}

static inline s3dint128_t_struct spew3d_to_int128_struct(int64_t v) {
    s3dint128_t_struct result;
    result.highsignificance = 0;
    result.lowsignificance = v;
    return result;
}

#ifndef S3D_INT128MULT
#if defined(__SIZEOF_INT128__) &&\
        !defined(SPEW3D_OPTION_DISABLE_EMULATE_INT128)
typedef __int128 s3dint128_t;
#define S3D_INT128MULT(x, y) ((s3dint128_t)x * (s3dint128_t)y)
#define S3D_INT128DIV(x, y) ((s3dint128_t)x / (s3dint128_t)y)
#define S3D_INT128FROM64(x) ((s3dint128_t)x)
#endif
#else
typedef s3dint128_t_struct s3dint128_t;
#define S3D_INT128MULT(x, y) (\
    sizeof(x) > 8 ? (spew3d_int128_mult((int128_t_struct)x,\
    (int128_t_struct)y)) :\
    (spew3d_int128_mult(spew3d_to_int128_struct(x),\
     spew3d_to_int128_struct(y))))
#define S3D_INT128DIV(x, y) (\
    sizeof(x) > 8 ? (spew3d_int128_div((int128_t_struct)x,\
    (int128_t_struct)y)) :\
    (spew3d_int128_mult(spew3d_to_int128_struct(x),\
     spew3d_to_int128_struct(y))))
#define S3D_INT128FROM64(x) (spew3d_to_int128_struct(x))
#endif

#endif  // SPEW3D_INT128_H_

