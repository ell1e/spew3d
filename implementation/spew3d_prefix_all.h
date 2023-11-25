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

// Some stuff that will be put at the start of spew3d.h before
// all external modules.

// XXX: Header-guard intentionally missing!

#ifndef _WIN32_WINNT
// Windows Vista
#define _WIN32_WINNT 0x0600
#endif

#if !defined(S3DEXP) && !defined(SPEW3D_OPTION_DISABLE_DLLEXPORT)
#if (defined(_WIN32) || defined(_WIN64))
#define S3DEXP __declspec(dllexport)
#if defined(__MINGW32__) || defined(__MINGW64__)
#define S3DHID __attribute__ ((visibility ("hidden")))
#endif
#else
#define S3DEXP __attribute__ ((visibility ("default")))
#define S3DHID __attribute__ ((visibility ("hidden")))
#endif
#endif

#define S3DLIKELY(x) __builtin_expect(!!(x), 1)
#define S3DUNLIKELY(x) __builtin_expect(!!(x), 0)

// Try to ensure 64bit file handling:
#define _TIME_BITS 64
#define _FILE_OFFSET_BITS 64
#ifndef __USE_LARGEFILE64
#define __USE_LARGEFILE64 1
#endif
#ifndef _LARGEFILE64_SOURCE
#define _LARGEFILE64_SOURCE
#endif
#define _LARGEFILE_SOURCE

// For <stb/stb_image.h>:
#define STBI_NO_STDIO
#define STBI_ONLY_JPEG
#define STBI_ONLY_PNG
#define STBI_ONLY_BMP
#define STBI_ONLY_TGA
#define STBI_ONLY_PNM
#ifdef SPEW3D_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#endif

// For <r128.h>:
#ifdef SPEW3D_IMPLEMENTATION
#define R128_IMPLEMENTATION
#endif

// For <miniz/miniz.h>:
#define MINIZ_NO_ZLIB_APIS
#define MINIZ_NO_ZLIB_COMPATIBLE_NAMES

// Some debug options:
#ifdef SPEW3D_DEBUG_OUTPUT
#define DEBUG_SPEW3D_FS
#define DEBUG_SPEW3D_VFS
#define DEBUG_SPEW3D_TEXTURE
#define DEBUG_SPEW3D_AUDIO_SINK
//#define DEBUG_SPEW3D_AUDIO_SINK_DATA
#define DEBUG_SPEW3D_AUDIO_DECODE
//#define DEBUG_SPEW3D_AUDIO_DECODE_DATA
//#define DEBUG_SPEW3D_AUDIO_DECODE_RESAMPLE
#endif

// Number types:
#if defined(__SIZEOF_INT128__) &&\
        !defined(SPEW3D_OPTION_DISABLE_EMULATE_INT128)
typedef __int128 s3dint128_t;
#else
typedef struct s3dint128_t_struct {
    uint64_t highsignificance;
    int64_t lowsignificance;
} s3dint128_t_struct;
typedef s3dint128_t_struct s3dint128_t;
#endif
#ifndef SPEW3D_FIXED_POINT
typedef double s3dnum_t;
#define S3D_METER (1.0)
#define S3D_NUMONE S3D_METER
#define S3D_NUMTODBL(x) ((double)x)
#define S3D_DBLTONUM(x) ((double)x)
#else
#include <stdint.h>
typedef int64_t s3dnum_t;
#ifndef S3D_METER
// Default to 12 bits of fractional part:
#define S3D_METER (4096)
#define S3D_NUMONE (4096)
#define S3D_NUMTODBL(x) ((double)x / (double)NUMONE)
#define S3D_DBLTONUM(x) ((s3d_num_t)((double)x * (double)NUMONE))
#endif
#endif
#ifndef S3D_BIGNUM_MAXFRACTIONDIGITS
#define S3D_BIGNUM_MAXFRACTIONDIGITS 200
#endif

// Some code is written with this assumption (e.g. spew3d_bigint.h):
#include <limits.h>
#include <stdint.h>
#if UINTPTR_MAX > UINT64_MAX
  #error "Some Spew3D code cannot handle this pointer size."
#endif

// Audio settings:
typedef int32_t s3d_asample_t;
#define S3D_ASAMPLE_MAX INT32_MAX
#define S3D_ASAMPLE_MIN INT32_MIN
#ifndef SPEW3D_SINK_AUDIOBUF_SAMPLES_ONECHAN
#define SPEW3D_SINK_AUDIOBUF_SAMPLES_ONECHAN (\
    256\
)
#endif
#ifdef SPEW3D_SINK_AUDIOBUF_BYTES
#error "SPEW3D_SINK_AUDIOBUF_BYTES already defined"
#else
#define SPEW3D_SINK_AUDIOBUF_BYTES (\
    SPEW3D_SINK_AUDIOBUF_SAMPLES_ONECHAN *\
    (int32_t)sizeof(s3d_asample_t)\
)
#endif

