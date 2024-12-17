/* Copyright (c) 2024, ellie/@ell1e & Spew3D Team (see AUTHORS.md).

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

#include <string.h>

typedef struct s3d_audio_resampler_internal {
    char **sourcebufptr;
    int *sourcebufsizeptr, *sourcebuffillptr;
    int input_samplerate, channels, output_samplerate;
    double resample_factor;

    char *decodeaheadbuf;
    int decodeaheadbuf_size, decodeaheadbuf_fillbytes;
} s3d_audio_resampler_internal;

S3DEXP s3d_audio_resampler *s3daudioresampler_New(
        char **sourcebufptr, int *sourceallocsizeptr,
        int *sourcefillbytesptr, int input_samplerate,
        int output_samplerate, int output_channels
        ) {
    s3d_audio_resampler_internal *e = malloc(sizeof(*e));
    if (!e)
        return NULL;
    memset(e, 0, sizeof(*e));
    s3d_audio_resampler *d = malloc(sizeof(*d));
    if (!d) {
        free(e);
        return NULL;
    }
    memset(d, 0, sizeof(*d));
    d->extra = e;
    e->sourcebufptr = sourcebufptr;
    e->sourcebufsizeptr = sourceallocsizeptr;
    e->sourcebuffillptr = sourcefillbytesptr;
    e->channels = output_channels;
    e->input_samplerate = input_samplerate;
    e->output_samplerate = output_samplerate;
    return d;
}

S3DEXP void s3d_audioresampler_SetNewTargetSamplerate(
        s3d_audio_resampler *d, int samplerate
        ) {
    d->extra->resample_factor = 0;
    d->extra->output_samplerate = samplerate;
}

S3DEXP void s3d_audioresampler_Free(
        s3d_audio_resampler *d
        ) {
    if (d == NULL)
        return;
    s3d_audio_resampler_internal *e = d->extra;
    free(e);
    free(d->resampledbuf);
    free(d);
}

S3DEXP int s3d_audioresampler_FillBufResampled(
        s3d_audio_resampler *d
        ) {
    s3d_audio_resampler_internal *e = d->extra;
    e->decodeaheadbuf = *e->sourcebufptr;
    e->decodeaheadbuf_size = *e->sourcebufsizeptr;
    e->decodeaheadbuf_fillbytes = *e->sourcebuffillptr;

    int frames_written = 0;
    if (e->decodeaheadbuf_fillbytes <= 0)
        return 1;
    const int want_resampled_buffered_frames = (
        e->output_samplerate
    );
    const int want_resampled_buffered_bytes = (
        want_resampled_buffered_frames *
        e->channels * sizeof(DECODEMIXTYPE)
    );
    int resampling = (e->input_samplerate !=
        e->output_samplerate);
    if (resampling && (
            !d->resampledbuf ||
            d->resampledbuf_fillbytes <
            want_resampled_buffered_bytes)) {
        if (e->resample_factor == 0) {
            if (e->input_samplerate != e->output_samplerate) {
                e->resample_factor =
                    ((double)e->output_samplerate) /
                    ((double)e->input_samplerate);
            } else {
                e->resample_factor = 1;
            }
        }
        assert(sizeof(DECODEMIXTYPE) == 2 ||
            sizeof(DECODEMIXTYPE) == 4);
        assert(d->resampledbuf_fillbytes >= 0);
        int buf_size_factor = ceil(fmax(2.0,
            e->resample_factor));
        int buf_size = (
            e->output_samplerate * sizeof(DECODEMIXTYPE) *
            e->channels * buf_size_factor
        ) + d->resampledbuf_fillbytes;
        assert(buf_size > 0);
        while (buf_size < (int)(e->input_samplerate *
                sizeof(DECODEMIXTYPE) *
                e->channels) +
                d->resampledbuf_fillbytes)
            buf_size *= 2;
        if (!d->resampledbuf ||
                d->resampledbuf_size < buf_size) {
            char *newresampledbuf = realloc(
                d->resampledbuf, buf_size
            );
            if (!newresampledbuf) {
                return 0;
            }
            d->resampledbuf = newresampledbuf;
            d->resampledbuf_size = buf_size;
        }

        #if defined(DEBUG_SPEW3D_AUDIO_RESAMPLE_DATA)
        assert(buf_size >= 0);
        assert(d->resampledbuf_fillbytes >= 0);
        printf(
            "spew3d_audio_decoder.c: debug: "
            "decoder addr=%p begin resample loop, freq "
            "%d -> %d, input total %d bytes, "
            "desired output total %d bytes, "
            "buf_size_factor %d, e->resample_factor %f, "
            "buf space for resample op %d bytes total (with "
            "%d already filled from previous operations)\n",
            d, e->input_samplerate, e->output_samplerate,
            d->resampledbuf_fillbytes,
            want_resampled_buffered_bytes,
            buf_size_factor, e->resample_factor,
            buf_size, d->resampledbuf_fillbytes
        );
        #endif
        int didoneresample = 0;
        while (d->resampledbuf_fillbytes <
                want_resampled_buffered_bytes &&
                e->decodeaheadbuf_fillbytes > 0) {
            int unresampled_input_bytes = e->decodeaheadbuf_fillbytes;
            int doresample_output_frames = (
                e->decodeaheadbuf_fillbytes / (sizeof(DECODEMIXTYPE) *
                e->channels)
            ) * e->resample_factor;
            /*int doresample_output_bufspace = (
                (e->decodeaheadbuf_fillbytes >= 1 ?
                    e->decodeaheadbuf_fillbytes : 1) * ceil(fmax(1.0,
                    e->resample_factor)));*/
            assert(doresample_output_frames >= 0);
            /*assert(doresample_output_bufspace >=
                doresample_frames * sizeof(DECODEMIXTYPE) *
                e->channels);*/
            if (doresample_output_frames == 0 && (
                    e->decodeaheadbuf_fillbytes /
                    (sizeof(DECODEMIXTYPE) *
                    e->channels)) > 0)
                doresample_output_frames = 1;
            int doresample_output_bytes = (
                doresample_output_frames * sizeof(DECODEMIXTYPE) *
                e->channels
            );
            if (doresample_output_bytes <= 0)
                break;

            #ifndef SPEW3D_OPTION_DISABLE_SDL
            SDL_AudioCVT cvt;
            memset(&cvt, 0, sizeof(cvt));
            SDL_BuildAudioCVT(
                &cvt, (sizeof(DECODEMIXTYPE) == 2 ? AUDIO_S16 :
                    AUDIO_S32), e->channels,
                e->input_samplerate,
                (sizeof(DECODEMIXTYPE) == 2 ? AUDIO_S16 :
                    AUDIO_S32), e->channels,
                e->output_samplerate
            );
            #if defined(DEBUG_SPEW3D_AUDIO_RESAMPLE_DATA)
            printf(
                "spew3d_audio_decoder.c: debug: "
                "decoder addr=%p sdl2 resample step %d->%d, "
                "sizeof(DECODEMIXTYPE)==%d, "
                "unresampled_input_bytes=%d, cvt.len_mult=%d, "
                "d->resampledbuf_size=%d, "
                "d->resampledbuf_fillbytes=%d\n",
                d, e->input_samplerate, e->output_samplerate,
                (int)sizeof(DECODEMIXTYPE),
                (int)unresampled_input_bytes, (int)cvt.len_mult,
                (int)d->resampledbuf_size,
                (int)d->resampledbuf_fillbytes
            );
            #endif
            if (unresampled_input_bytes * cvt.len_mult > (
                    d->resampledbuf_size -
                    d->resampledbuf_fillbytes)) {
                if (didoneresample)
                    break;  // Just stop here.
                // SDL2 wants really strangely huge buffers sometimes.
                // Not much we can do except comply:
                int new_size = (unresampled_input_bytes *
                    cvt.len_mult) +
                    d->resampledbuf_fillbytes;
                char *newresampledbuf = realloc(
                    d->resampledbuf, buf_size
                );
                if (!newresampledbuf) {
                    return 0;
                }
                d->resampledbuf = newresampledbuf;
                d->resampledbuf_size = buf_size;
            }
            cvt.len = unresampled_input_bytes;
            cvt.buf = (
                (unsigned char *)d->resampledbuf +
                (unsigned int)d->resampledbuf_fillbytes);
            memcpy(cvt.buf, e->decodeaheadbuf, cvt.len);
            SDL_ConvertAudio(&cvt);
            if (e->decodeaheadbuf_fillbytes < cvt.len) {
                memmove(
                    e->decodeaheadbuf,
                    e->decodeaheadbuf + cvt.len,
                    e->decodeaheadbuf_fillbytes - cvt.len
                );
                e->decodeaheadbuf_fillbytes -= cvt.len;
            } else {
                e->decodeaheadbuf_fillbytes = 0;
            }
            assert(cvt.len_cvt > 0);
            assert(cvt.len_cvt <= (d->resampledbuf_size -
                d->resampledbuf_fillbytes));
            d->resampledbuf_fillbytes += cvt.len_cvt;
            assert(d->resampledbuf_fillbytes <
                d->resampledbuf_size);
            #else
            fprintf(  // XXX: FIXME !!!
                stderr, "spew3d_audio_decoder.c: error: "
                "resampling code path not implemented");
            _exit(1);
            #endif
            didoneresample = 1;
        }
    } else if (d->resampledbuf_fillbytes <
            want_resampled_buffered_bytes) {
        int copybytes = want_resampled_buffered_bytes;
        if (copybytes > e->decodeaheadbuf_fillbytes)
            copybytes > e->decodeaheadbuf_fillbytes;
        int wantsize = d->resampledbuf_fillbytes +
            copybytes;
        if (wantsize > d->resampledbuf_size) {
             char *newresampledbuf = realloc(
                d->resampledbuf, wantsize
            );
            if (!newresampledbuf) {
                return 0;
            }
            d->resampledbuf = newresampledbuf;
            d->resampledbuf_size = wantsize;
        }
        if (copybytes > 0) {
            memcpy(d->resampledbuf + d->resampledbuf_fillbytes,
                e->decodeaheadbuf, e->decodeaheadbuf_fillbytes);
            if (copybytes < e->decodeaheadbuf_fillbytes) {
                memmove(e->decodeaheadbuf,
                    e->decodeaheadbuf + copybytes,
                    (e->decodeaheadbuf_fillbytes - copybytes));
            }
            e->decodeaheadbuf_fillbytes -= copybytes;
        }
    }
    *e->sourcebuffillptr = e->decodeaheadbuf_fillbytes;
    return 1;
}

#endif  // SPEW3D_IMPLEMENTATION

