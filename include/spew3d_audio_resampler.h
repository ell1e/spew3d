/* Copyright (c) 2023-2024, ellie/@ell1e & Spew3D Team (see AUTHORS.md).

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
    
#ifndef SPEW3D_AUDIO_RESAMPLER_H_
#define SPEW3D_AUDIO_RESAMPLER_H_

typedef struct s3d_audio_resampler_internal
    s3d_audio_resampler_internal;

typedef struct s3d_audio_resampler {
    char *resampledbuf;
    int resampledbuf_size, resampledbuf_fillbytes;

    s3d_audio_resampler_internal *extra;
} s3d_audio_resampler;

S3DEXP s3d_audio_resampler *s3daudioresampler_New(
    char **sourcebufptr, int *sourceallocsizeptr,
    int *sourcefillbytesptr, int input_samplerate,
    int output_samplerate, int output_channels
);

S3DEXP void s3d_audioresampler_SetNewTargetSamplerate(
    s3d_audio_resampler *d, int sample_rate
);

S3DEXP int s3d_audioresampler_FillBufResampled(
    s3d_audio_resampler *d
);

S3DEXP void s3d_audioresampler_Free(
    s3d_audio_resampler *d
);

#endif  // SPEW3D_AUDIO_RESAMPLER_H_
