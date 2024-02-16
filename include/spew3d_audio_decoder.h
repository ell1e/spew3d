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
    
#ifndef SPEW3D_AUDIO_DECODER_H_
#define SPEW3D_AUDIO_DECODER_H_

#include <stdint.h>

typedef struct s3d_audio_decoder s3d_audio_decoder;

S3DEXP s3d_audio_decoder *audiodecoder_NewFromFile(
    const char *filepath
);

S3DEXP s3d_audio_decoder *audiodecoder_NewFromFileEx(
    const char *filepath, int vfsflags
);

S3DEXP int s3d_audiodecoder_SetChannelAdjustTo(
    s3d_audio_decoder *d, int channels
);

S3DEXP int s3d_audiodecoder_GetSourceSampleRate(
    s3d_audio_decoder *d
);

S3DEXP int s3d_audiodecoder_GetSourceChannels(
    s3d_audio_decoder *d
);

S3DEXP int s3d_audiodecoder_GetOutputChannels(
    s3d_audio_decoder *d
);

S3DEXP int s3d_audiodecoder_SetResampleTo(
    s3d_audio_decoder *d, int samplerate
);

S3DEXP int s3d_audiodecoder_Decode(
    s3d_audio_decoder *d, char *output, int frames,
    int *out_haderror
);

S3DEXP void s3d_audiodecoder_ResetToStart(s3d_audio_decoder *d);

S3DEXP void s3d_audiodecoder_Destroy(s3d_audio_decoder *d);

S3DEXP int s3d_audiodecoder_HadError(s3d_audio_decoder *d);

#endif  // SPEW3D_AUDIO_DECODER_H_

