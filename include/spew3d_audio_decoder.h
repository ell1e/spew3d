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
    
#ifndef SPEW3D_AUDIO_DECODER_H_
#define SPEW3D_AUDIO_DECODER_H_

#include <stdint.h>

typedef struct s3daudiodecoder s3daudiodecoder;

s3daudiodecoder *audiodecoder_NewFromFile(
    const char *filepath
);

s3daudiodecoder *audiodecoder_NewFromFileEx(
    const char *filepath, int vfsflags
);

int s3d_audiodecoder_SetChannelAdjustTo(
    s3daudiodecoder *d, int channels
);

int s3d_audiodecoder_GetSourceSampleRate(
    s3daudiodecoder *d
);

int s3d_audiodecoder_GetSourceChannels(
    s3daudiodecoder *d
);

int s3d_audiodecoder_GetOutputChannels(
    s3daudiodecoder *d
);

int s3d_audiodecoder_SetResampleTo(
    s3daudiodecoder *d, int samplerate
);

int s3d_audiodecoder_Decode(
    s3daudiodecoder *d, char *output, int frames,
    int *out_haderror
);

void s3d_audiodecoder_ResetToStart(s3daudiodecoder *d);

void s3d_audiodecoder_Destroy(s3daudiodecoder *d);

int s3d_audiodecoder_HadError(s3daudiodecoder *d);

#endif  // SPEW3D_AUDIO_DECODER_H_

