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

#ifdef SPEW3D_IMPLEMENTATION

#include <assert.h>
#include <inttypes.h>
#if defined(_WIN32) || defined(_WIN64)
#include <malloc.h>
#else
#include <alloca.h>
#endif
#ifndef SPEW3D_OPTION_DISABLE_SDL
#include <SDL2/SDL.h>
#endif
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define DECODEMIXTYPE s3d_asample_t

typedef struct s3daudiodecoder {
    char *audiopath;
    int input_channels, input_samplerate,
        output_samplerate, output_channels;
    int vfsflags;
    SPEW3DVFS_FILE *vfshandle;
    int vfserror;

    int decode_endoffile;
    char *decodeaheadbuf;
    int decodeaheadbuf_fillbytes;
    char *decodeaheadbuf_resampled;
    int decodeaheadbuf_resampled_size,
        decodeaheadbuf_resampled_fillbytes;
    double resample_factor;

    drmp3 *_mp3decode;
    drwav *_wavdecode;
    drflac *_flacdecode;
} s3daudiodecoder;

static int s3d_audiodecoder_FillDecodeAhead(s3daudiodecoder *d);

s3daudiodecoder *audiodecoder_NewFromFile(
        const char *filepath
        ) {
    return audiodecoder_NewFromFileEx(filepath, 0);
}

s3daudiodecoder *audiodecoder_NewFromFileEx(
        const char *filepath, int vfsflags
        ) {
    int _exists = 0;
    int _existsfserr = 0;
    if (!spew3d_vfs_Exists(filepath, vfsflags,
            &_exists, &_existsfserr) || !_exists) {
        return NULL;
    }
    s3daudiodecoder *d = malloc(sizeof(*d));
    if (!d)
        return NULL;
    memset(d, 0, sizeof(*d));
    d->audiopath = strdup(filepath);
    if (!d->audiopath) {
        free(d);
        return NULL;
    }
    d->vfsflags = vfsflags;
    return d;
}

int s3d_audiodecoder_GetSourceSampleRate(
        s3daudiodecoder *d
        ) {
    if (d->input_samplerate == 0 && !d->vfshandle &&
            !d->_mp3decode && !d->_wavdecode &&
            !d->_flacdecode &&
            !d->vfserror) {
        // Probably was never even touched by decoders yet.
        // Do so, so we get the basic file info:
        if (!s3d_audiodecoder_FillDecodeAhead(d))
            return 0;
    }
    return d->input_samplerate;
}

int s3d_audiodecoder_GetOutputChannels(
        s3daudiodecoder *d
        ) {
    if (d->output_channels == 0 && !d->vfshandle &&
            !d->_mp3decode && !d->_wavdecode &&
            !d->_flacdecode &&
            !d->vfserror) {
        // Probably was never even touched by decoders yet.
        // Do so, so we get the basic file info:
        if (!s3d_audiodecoder_FillDecodeAhead(d))
            return 0;
    }
    assert(d->output_channels > 0 || d->vfserror);
    return d->output_channels;
}

int s3d_audiodecoder_GetSourceChannels(
        s3daudiodecoder *d
        ) {
    if (d->input_channels == 0 && !d->vfshandle &&
            !d->_mp3decode && !d->_wavdecode &&
            !d->_flacdecode &&
            !d->vfserror) {
        // Probably was never even touched by decoders yet.
        // Do so, so we get the basic file info:
        if (!s3d_audiodecoder_FillDecodeAhead(d))
            return 0;
    }
    return d->input_channels;
}

int s3d_audiodecoder_SetChannelAdjustTo(
        s3daudiodecoder *d, int channels
        ) {
    if (channels < 1 || channels > 10)
        return 0;
    d->output_channels = channels;
    return 1;
}

int s3d_audiodecoder_SetResampleTo(
        s3daudiodecoder *d, int samplerate
        ) {
    if (samplerate < 10000 || samplerate > 100000)
        return 0;
    d->resample_factor = 0;
    d->output_samplerate = samplerate;
    return 1;
}

size_t _drmp3drwavdrflac_read_cb(
        void *ud, void *pBufferOut,
        size_t bytesToRead) {
    s3daudiodecoder *d = ud;
    if (!d->vfshandle || d->vfserror)
        return 0;
    size_t result = spew3d_vfs_fread(
        pBufferOut, 1, bytesToRead, d->vfshandle
    );
    if (result == 0 && spew3d_vfs_ferror(d->vfshandle))
        d->vfserror = 1;
    return result;
}

uint32_t _drmp3_seek_cb(void *ud,
        int offset, drmp3_seek_origin origin) {
    s3daudiodecoder *d = ud;
    if (!d->vfshandle || d->vfserror)
        return 0;
    if (origin == drmp3_seek_origin_start) {
        return (spew3d_vfs_fseek(d->vfshandle, offset) >= 0);
    } else {
        int64_t oldoffset = spew3d_vfs_ftell(d->vfshandle);
        if (oldoffset < 0)
            return 0;
        return (spew3d_vfs_fseek(d->vfshandle, oldoffset + offset) >= 0);
    }
}

uint32_t _drwav_seek_cb(void *ud,
        int offset, drwav_seek_origin origin) {
    s3daudiodecoder *d = ud;
    if (!d->vfshandle || d->vfserror)
        return 0;
    if (origin == drwav_seek_origin_start) {
        return (spew3d_vfs_fseek(d->vfshandle, offset) >= 0);
    } else {
        int64_t oldoffset = spew3d_vfs_ftell(d->vfshandle);
        if (oldoffset < 0)
            return 0;
        return (spew3d_vfs_fseek(d->vfshandle, oldoffset + offset) >= 0);
    }
}

uint32_t _drflac_seek_cb(void *ud,
        int offset, drflac_seek_origin origin) {
    s3daudiodecoder *d = ud;
    if (!d->vfshandle || d->vfserror)
        return 0;
    if (origin == drflac_seek_origin_start) {
        return (spew3d_vfs_fseek(d->vfshandle, offset) >= 0);
    } else {
        int64_t oldoffset = spew3d_vfs_ftell(d->vfshandle);
        if (oldoffset < 0)
            return 0;
        return (spew3d_vfs_fseek(d->vfshandle, oldoffset + offset) >= 0);
    }
}

static void _expand_s16_to_s32(char *buf, uint64_t samples) {
    __attribute__((__may_alias__)) char *src = (
        buf + ((samples - 1) * sizeof(int16_t))
    );
    __attribute__((__may_alias__)) char *p = (
        buf + ((samples - 1) * sizeof(int32_t))
    );
    __attribute__((__may_alias__)) char *pend = buf - sizeof(int32_t);
    while (p != pend) {
        int64_t orig_val = *((int16_t *)src);
        int64_t new_val = orig_val * (int64_t)INT32_MAX;
        new_val /= (int64_t)INT16_MAX;
        if (new_val > (int64_t)INT32_MAX)
            new_val = (int64_t)INT32_MAX;
        else if (new_val < (int64_t)INT32_MIN)
            new_val = (int64_t)INT32_MIN;
        *((int32_t *)p) = new_val;
        //printf("orig_val: %" PRId64
        //    ", new_val: %" PRId64
        //    ", orig_val(float): %f, new_val(float): %f\n",
        //    orig_val, new_val, ((double)orig_val/
        //    (double)INT16_MAX), ((double)new_val/
        //    (double)INT32_MAX));
        src -= sizeof(int16_t);
        p -= sizeof(int32_t);
        assert(src > pend);
        assert(p >= pend);
    }
}

static int s3d_audiodecoder_FillDecodeAhead(s3daudiodecoder *d) {
    if (d->decode_endoffile)
        return 1;
    if (d->vfserror)
        return 0;

    if (!d->vfshandle && !d->vfserror) {
        d->vfshandle = spew3d_vfs_fopen(d->audiopath, "rb", d->vfsflags);
        if (!d->vfshandle) {
            d->vfserror = 1;
            return 0;
        }
    }
    if (!d->_mp3decode && !d->_wavdecode &&
            !d->_flacdecode) {
        d->_wavdecode = malloc(sizeof(*d->_wavdecode));
        if (!d->_wavdecode) {
            d->vfserror = 1;
            return 0;
        }
        memset(d->_wavdecode, 0, sizeof(*d->_wavdecode));
        if (d->vfshandle) {
            if (spew3d_vfs_fseek(d->vfshandle, 0) < 0) {
                free(d->_wavdecode);
                d->_wavdecode = NULL;
                d->vfserror = 1;
                return 0;
            }
        }
        if (drwav_init(
                d->_wavdecode, _drmp3drwavdrflac_read_cb,
                _drwav_seek_cb, d, NULL
                )) {
            #if defined(DEBUG_SPEW3D_AUDIO_DECODE)
            printf(
                "spew3d_audio_decoder.c: debug: decoder "
                "addr=%p wav: "
                "opened for decoding: %s\n",
                d, d->audiopath
            );
            #endif
            if (d->_wavdecode->channels < 1) {
                drwav_uninit(d->_wavdecode);
                free(d->_wavdecode);
                d->_wavdecode = NULL;
                d->vfserror = 1;
                return 0;
            }
            d->input_samplerate = d->_wavdecode->sampleRate;
            if (d->input_samplerate < 10000 ||
                    d->input_samplerate > 100000) {
                drwav_uninit(d->_wavdecode);
                free(d->_wavdecode);
                d->_wavdecode = NULL;
                d->vfserror = 1;
                return 0;
            }
            if (d->output_channels == 0)
                d->output_channels = d->_wavdecode->channels;
            if (d->output_samplerate == 0)
                d->output_samplerate = d->input_samplerate;
            d->input_channels = d->_wavdecode->channels;
        } else {
            free(d->_wavdecode);
            d->_wavdecode = NULL;
        }
    }
    if (!d->_mp3decode && !d->_wavdecode &&
            !d->_flacdecode) {
        if (d->vfshandle)
            if (spew3d_vfs_fseek(d->vfshandle, 0) < 0) {
                d->vfserror = 1;
                return 0;
            }
        if ((d->_flacdecode = drflac_open(
                _drmp3drwavdrflac_read_cb,
                _drflac_seek_cb, d, NULL
                )) != NULL) {
            #if defined(DEBUG_SPEW3D_AUDIO_DECODE)
            printf(
                "spew3d_audio_decoder.c: debug: decoder "
                "addr=%p flac: "
                "opened for decoding: %s\n",
                d, d->audiopath
            );
            #endif
            if (d->_flacdecode->channels < 1) {
                drflac_close(d->_flacdecode);
                d->_flacdecode = NULL;
                d->vfserror = 1;
                return 0;
            }
            d->input_samplerate = d->_flacdecode->sampleRate;
            if (d->input_samplerate < 10000 ||
                    d->input_samplerate > 100000) {
                drflac_close(d->_flacdecode);
                d->_flacdecode = NULL;
                d->vfserror = 1;
                return 0;
            }
            if (d->output_channels == 0)
                d->output_channels = d->_flacdecode->channels;
            if (d->output_samplerate == 0)
                d->output_samplerate = d->input_samplerate;
            d->input_channels = d->_flacdecode->channels;
        }
    }
    if (!d->_mp3decode && !d->_wavdecode &&
            !d->_flacdecode) {
        d->_mp3decode = malloc(sizeof(*d->_mp3decode));
        if (!d->_mp3decode) {
            d->vfserror = 1;
            return 0;
        }
        memset(d->_mp3decode, 0, sizeof(*d->_mp3decode));
        if (d->vfshandle) {
            if (spew3d_vfs_fseek(d->vfshandle, 0) < 0) {
                free(d->_mp3decode);
                d->_mp3decode = NULL;
                d->vfserror = 1;
                return 0;
            }
        }
        if (drmp3_init(
                d->_mp3decode, _drmp3drwavdrflac_read_cb,
                _drmp3_seek_cb, d, NULL
                )) {
            #if defined(DEBUG_SPEW3D_AUDIO_DECODE)
            printf(
                "spew3d_audio_decoder.c: debug: decoder "
                "addr=%p mp3: "
                "opened for decoding: %s\n",
                d, d->audiopath
            );
            #endif
            if (d->_mp3decode->channels < 1) {
                drmp3_uninit(d->_mp3decode);
                free(d->_mp3decode);
                d->_mp3decode = NULL;
                d->vfserror = 1;
                return 0;
            }
            d->input_samplerate = d->_mp3decode->sampleRate;
            if (d->input_samplerate < 10000 ||
                    d->input_samplerate > 100000) {
                drmp3_uninit(d->_mp3decode);
                free(d->_mp3decode);
                d->_mp3decode = NULL;
                d->vfserror = 1;
                return 0;
            }
            if (d->output_channels == 0)
                d->output_channels = d->_mp3decode->channels;
            if (d->output_samplerate == 0)
                d->output_samplerate = d->input_samplerate;
            d->input_channels = d->_mp3decode->channels;
        } else {
            free(d->_mp3decode);
            d->_mp3decode = NULL;
        }
    }
    if (d->vfserror || (!d->_mp3decode && !d->_wavdecode &&
            !d->_flacdecode)) {
        d->vfserror = 1;
        return 0;
    }
    if (!d->decodeaheadbuf) {
        assert(d->input_samplerate > 0);
        assert(d->input_channels > 0);
        d->decodeaheadbuf = malloc(
            d->input_samplerate * sizeof(DECODEMIXTYPE) *
                d->input_channels
        );
        if (!d->decodeaheadbuf) {
            d->decodeaheadbuf_fillbytes = 0;
            d->vfserror = 1;
            return 0;
        }
    }
    int want_to_read_bytes = (
        d->input_samplerate * sizeof(DECODEMIXTYPE) * d->input_channels
    ) - d->decodeaheadbuf_fillbytes;
    int want_to_read_frames = (
        want_to_read_bytes / (sizeof(DECODEMIXTYPE) * d->input_channels)
    );
    if (want_to_read_frames <= 0) {
        return 1;
    }
    uint64_t read_frames = 0;
    if (d->_mp3decode) {
        assert(
            (int)want_to_read_bytes +
                (int)d->decodeaheadbuf_fillbytes <=
            (int)d->input_samplerate * (int)sizeof(DECODEMIXTYPE) *
                (int)d->input_channels
        );
        assert(want_to_read_frames > 0);
        assert(
            (int)want_to_read_frames * (int)sizeof(DECODEMIXTYPE) *
                (int)d->input_channels +
                d->decodeaheadbuf_fillbytes <=
            (int)d->input_samplerate * (int)sizeof(DECODEMIXTYPE) *
                (int)d->input_channels
        );
        read_frames = drmp3_read_pcm_frames_s16(
            d->_mp3decode, want_to_read_frames,
            (drmp3_int16 *)((char *)d->decodeaheadbuf +
            d->decodeaheadbuf_fillbytes)
        );
        if (sizeof(DECODEMIXTYPE) == 2) {
            // 16bit int audio, nothing to do.
        } else if (sizeof(DECODEMIXTYPE) == 4) {
            // 32bit int audio, so we have to expand this.
            _expand_s16_to_s32(
                ((char *)d->decodeaheadbuf +
                    d->decodeaheadbuf_fillbytes),
                read_frames * d->input_channels
            );
        } else {
            fprintf(stderr,
                "spew3d_audio_decoder.c: error: "
                "unsupported DECODEMIXTYPE");
        }

        #if defined(DEBUG_SPEW3D_AUDIO_DECODE_DATA)
        printf(
            "spew3d_audio_decoder.c: debug: decoder "
            "addr=%p mp3: "
            "frames=%d(%dB) fillbytes(after)=%d/%d\n",
            d, (int)read_frames,
            (int)(read_frames * sizeof(DECODEMIXTYPE) *
            d->input_channels),
            (int)(d->decodeaheadbuf_fillbytes +
            read_frames * sizeof(DECODEMIXTYPE) *
            d->input_channels),
            (int)(d->input_samplerate * sizeof(DECODEMIXTYPE) *
            d->input_channels)
        );
        // Debug print some contents:
        char *printstart = ((char *)d->decodeaheadbuf +
            d->decodeaheadbuf_fillbytes);
        int gotbytes = (read_frames *
            sizeof(DECODEMIXTYPE) * d->input_channels);
        int printlen = gotbytes;
        if (printlen > 32) printlen = 32;
        printf(
            "spew3d_audio_decoder.c: debug: decoder "
            "addr=%p mp3: decoded %d bytes, an excerpt: ",
            d, gotbytes);
        int k = 0;
        while (k < printlen) {
            uint8_t byte = *(printstart + k);
            char hexbuf[3];
            snprintf(hexbuf, sizeof(hexbuf), "%x", (int)byte);
            if (strlen(hexbuf) < 2) {
                hexbuf[2] = '\0';
                hexbuf[1] = hexbuf[0];
                hexbuf[0] = '0';
            }
            printf("%s", hexbuf);
            k++;
        }
        printf("[END]\n");
        #endif
    } else if (d->_wavdecode) {
        read_frames = drwav_read_pcm_frames_s16(
            d->_wavdecode, want_to_read_frames,
            (drwav_int16 *)((char *)d->decodeaheadbuf +
            d->decodeaheadbuf_fillbytes)
        );
        if (sizeof(DECODEMIXTYPE) == 2) {
            // 16bit int audio, nothing to do.
        } else if (sizeof(DECODEMIXTYPE) == 4) {
            // 32bit int audio, so we have to expand this.
            _expand_s16_to_s32(
                ((char *)d->decodeaheadbuf +
                    d->decodeaheadbuf_fillbytes),
                read_frames * d->input_channels
            );
        } else {
            fprintf(stderr,
                "spew3d_audio_decoder.c: error: "
                "unsupported DECODEMIXTYPE");
        }

        #if defined(DEBUG_SPEW3D_AUDIO_DECODE_DATA)
        printf(
            "spew3d_audio_decoder.c: debug: decoder "
            "addr=%p wav: "
            "frames=%d(%dB) fillbytes(after)=%d/%d\n",
            d, (int)read_frames,
            (int)(read_frames * sizeof(DECODEMIXTYPE) * d->input_channels),
            (int)(d->decodeaheadbuf_fillbytes +
            read_frames * sizeof(DECODEMIXTYPE) * d->input_channels),
            (int)(d->input_samplerate * sizeof(DECODEMIXTYPE) *
            d->input_channels)
        );
        #endif
    } else if (d->_flacdecode) {
        read_frames = drflac_read_pcm_frames_s16(
            d->_flacdecode, want_to_read_frames,
            (drflac_int16 *)((char *)d->decodeaheadbuf +
            d->decodeaheadbuf_fillbytes)
        );
        if (sizeof(DECODEMIXTYPE) == 2) {
            // 16bit int audio, nothing to do.
        } else if (sizeof(DECODEMIXTYPE) == 4) {
            // 32bit int audio, so we have to expand this.
            _expand_s16_to_s32(
                ((char *)d->decodeaheadbuf +
                    d->decodeaheadbuf_fillbytes),
                read_frames * d->input_channels
            );
        } else {
            fprintf(stderr,
                "spew3d_audio_decoder.c: error: "
                "unsupported DECODEMIXTYPE");
        }

        #if defined(DEBUG_SPEW3D_AUDIO_DECODE_DATA)
        printf(
            "spew3d_audio_decoder.c: debug: decoder "
            "addr=%p flac: "
            "frames=%d(%dB) fillbytes(after)=%d/%d\n",
            d, (int)read_frames,
            (int)(read_frames * sizeof(DECODEMIXTYPE) * d->input_channels),
            (int)(d->decodeaheadbuf_fillbytes +
            read_frames * sizeof(DECODEMIXTYPE) * d->input_channels),
            (int)(d->input_samplerate * sizeof(DECODEMIXTYPE) *
            d->input_channels)
        );
        #endif
    } else {
        #if defined(DEBUG_SPEW3D_AUDIO_DECODE)
        fprintf(stderr, "spew3d_audio_decoder.c: warning: "
            "unknown decode type, failed to determine "
            "audio format\n");
        #endif
        return 0;
    }
    if (read_frames == 0) {
        d->decode_endoffile = 1;
        if (d->_mp3decode) {
            drmp3_uninit(d->_mp3decode);
            free(d->_mp3decode);
            d->_mp3decode = NULL;
        }
        if (d->_wavdecode) {
            drwav_uninit(d->_wavdecode);
            free(d->_wavdecode);
            d->_wavdecode = NULL;
        }
        if (d->_flacdecode) {
            drflac_close(d->_flacdecode);
            d->_flacdecode = NULL;
        }
    }
    d->decodeaheadbuf_fillbytes += (
        read_frames * sizeof(DECODEMIXTYPE) * d->input_channels
    );
    return 1;
}

static int s3d_audiodecoder_FillDecodeAheadResampled(
        s3daudiodecoder *d
        ) {
    int frames_written = 0;
    if (d->vfserror || !s3d_audiodecoder_FillDecodeAhead(d)) {
        d->vfserror = 1;
        return 0;
    }
    if (d->decodeaheadbuf_fillbytes <= 0)
        return 1;
    const int want_resampled_buffered_frames = (
        d->output_samplerate
    );
    const int want_resampled_buffered_bytes = (
        want_resampled_buffered_frames *
        d->output_channels * sizeof(DECODEMIXTYPE)
    );
    int resampling = (d->input_samplerate !=
        d->output_samplerate);
    if (resampling && (
            !d->decodeaheadbuf_resampled ||
            d->decodeaheadbuf_resampled_fillbytes <
            want_resampled_buffered_bytes)) {
        if (d->resample_factor == 0) {
            if (d->input_samplerate != d->output_samplerate) {
                d->resample_factor =
                    ((double)d->output_samplerate) /
                    ((double)d->input_samplerate);
            } else {
                d->resample_factor = 1;
            }
        }
        assert(sizeof(DECODEMIXTYPE) == 2 ||
            sizeof(DECODEMIXTYPE) == 4);
        assert(d->decodeaheadbuf_resampled_fillbytes >= 0);
        int buf_size_factor = ceil(fmax(2.0,
                d->resample_factor));
        int buf_size = (
            d->output_samplerate * sizeof(DECODEMIXTYPE) *
            d->output_channels * buf_size_factor
        ) + d->decodeaheadbuf_resampled_fillbytes;
        assert(buf_size > 0);
        while (buf_size < (int)(d->input_samplerate *
                sizeof(DECODEMIXTYPE) *
                d->output_channels) +
                d->decodeaheadbuf_resampled_fillbytes)
            buf_size *= 2;
        if (!d->decodeaheadbuf_resampled ||
                d->decodeaheadbuf_resampled_size < buf_size) {
            char *newresampledbuf = realloc(
                d->decodeaheadbuf_resampled, buf_size
            );
            if (!newresampledbuf) {
                d->vfserror = 1;
                return 0;
            }
            d->decodeaheadbuf_resampled = newresampledbuf;
            d->decodeaheadbuf_resampled_size = buf_size;
        }

        #if defined(DEBUG_SPEW3D_AUDIO_DECODE_RESAMPLE)
        assert(buf_size >= 0);
        assert(d->decodeaheadbuf_resampled_fillbytes >= 0);
        printf(
            "spew3d_audio_decoder.c: debug: "
            "decoder addr=%p begin resample loop, freq "
            "%d -> %d, input total %d bytes, "
            "desired output total %d bytes, "
            "buf_size_factor %d, d->resample_factor %f, "
            "buf space for resample op %d bytes total (with "
            "%d already filled from previous operations)\n",
            d, d->input_samplerate, d->output_samplerate,
            d->decodeaheadbuf_fillbytes,
            want_resampled_buffered_bytes,
            buf_size_factor, d->resample_factor,
            buf_size, d->decodeaheadbuf_resampled_fillbytes
        );
        #endif
        int didoneresample = 0;
        while (d->decodeaheadbuf_resampled_fillbytes <
                want_resampled_buffered_bytes) {
            int unresampled_input_bytes = d->decodeaheadbuf_fillbytes;
            int doresample_output_frames = (
                d->decodeaheadbuf_fillbytes / (sizeof(DECODEMIXTYPE) *
                d->output_channels)
            ) * d->resample_factor;
            /*int doresample_output_bufspace = (
                (d->decodeaheadbuf_fillbytes >= 1 ?
                    d->decodeaheadbuf_fillbytes : 1) * ceil(fmax(1.0,
                    d->resample_factor)));*/
            assert(doresample_output_frames >= 0);
            /*assert(doresample_output_bufspace >=
                doresample_frames * sizeof(DECODEMIXTYPE) *
                d->output_channels);*/
            if (doresample_output_frames == 0 && (
                    d->decodeaheadbuf_fillbytes /
                    (sizeof(DECODEMIXTYPE) *
                    d->output_channels)) > 0)
                doresample_output_frames = 1;
            int doresample_output_bytes = (
                doresample_output_frames * sizeof(DECODEMIXTYPE) *
                d->output_channels
            );
            if (doresample_output_bytes <= 0)
                break;

            #ifndef SPEW3D_OPTION_DISABLE_SDL
            SDL_AudioCVT cvt;
            memset(&cvt, 0, sizeof(cvt));
            SDL_BuildAudioCVT(
                &cvt, (sizeof(DECODEMIXTYPE) == 2 ? AUDIO_S16 :
                    AUDIO_S32), d->output_channels,
                d->input_samplerate,
                (sizeof(DECODEMIXTYPE) == 2 ? AUDIO_S16 :
                    AUDIO_S32), d->output_channels,
                d->output_samplerate
            );
            #if defined(DEBUG_SPEW3D_AUDIO_DECODE_RESAMPLE)
            printf(
                "spew3d_audio_decoder.c: debug: "
                "decoder addr=%p sdl2 resample step %d->%d, "
                "sizeof(DECODEMIXTYPE)==%d, "
                "unresampled_input_bytes=%d, cvt.len_mult=%d, "
                "d->decodeaheadbuf_resampled_size=%d, "
                "d->decodeaheadbuf_resampled_fillbytes=%d\n",
                d, d->input_samplerate, d->output_samplerate,
                (int)sizeof(DECODEMIXTYPE),
                (int)unresampled_input_bytes, (int)cvt.len_mult,
                (int)d->decodeaheadbuf_resampled_size,
                (int)d->decodeaheadbuf_resampled_fillbytes
            );
            #endif
            if (unresampled_input_bytes * cvt.len_mult > (
                    d->decodeaheadbuf_resampled_size -
                    d->decodeaheadbuf_resampled_fillbytes)) {
                if (didoneresample)
                    break;  // Just stop here.
                // SDL2 wants really strangely huge buffers sometimes.
                // Not much we can do except comply:
                int new_size = (unresampled_input_bytes *
                    cvt.len_mult) +
                    d->decodeaheadbuf_resampled_fillbytes;
                char *newresampledbuf = realloc(
                    d->decodeaheadbuf_resampled, buf_size
                );
                if (!newresampledbuf) {
                    d->vfserror = 1;
                    return 0;
                }
                d->decodeaheadbuf_resampled = newresampledbuf;
                d->decodeaheadbuf_resampled_size = buf_size;
            }
            cvt.len = unresampled_input_bytes;
            cvt.buf = (
                (unsigned char *)d->decodeaheadbuf_resampled +
                (unsigned int)d->decodeaheadbuf_resampled_fillbytes);
            memcpy(cvt.buf, d->decodeaheadbuf, cvt.len);
            SDL_ConvertAudio(&cvt);
            d->decodeaheadbuf_fillbytes -= cvt.len;
            if (d->decodeaheadbuf_fillbytes > 0) {
                memmove(
                    d->decodeaheadbuf,
                    d->decodeaheadbuf + cvt.len,
                    d->decodeaheadbuf_fillbytes - cvt.len
                );
            }
            assert(cvt.len_cvt > 0);
            assert(cvt.len_cvt <= (d->decodeaheadbuf_resampled_size -
                d->decodeaheadbuf_resampled_fillbytes));
            d->decodeaheadbuf_fillbytes -= cvt.len;
            d->decodeaheadbuf_resampled_fillbytes += cvt.len_cvt;
            #else
            fprintf(  // XXX: FIXME !!!
                stderr, "spew3d_audio_decoder.c: error: "
                "resampling code path not implemented");
            _exit(1);
            #endif
            didoneresample = 1;
        }
    }
    return 1;
}

int s3d_audiodecoder_Decode(
        s3daudiodecoder *d, char *output, int frames,
        int *out_haderror
        ) {
    // Ensure basic data is set on our source stream:
    if (d->input_samplerate == 0) {
        if (d->vfserror ||
                !s3d_audiodecoder_FillDecodeAheadResampled(
                d) || d->input_samplerate == 0) {
            // We failed to obtain basic data.
            *out_haderror = 1;
            return 0;
        }
    }

    // Determine what we want to do:
    char *output_unadjusted_channels = output;
    if (d->input_channels != d->output_channels) {
        int needed_channeladjust_bytes = (
            (int)(d->input_channels *
            sizeof(DECODEMIXTYPE) * frames)
        );
        //output_unadjusted_channels =
    }
    const int resampling = (d->input_samplerate !=
        d->output_samplerate);
    int frames_written = 0;

    while (frames_written < frames) {
        if (d->vfserror ||
                !s3d_audiodecoder_FillDecodeAheadResampled(
                d)) {
            *out_haderror = 1;
            return 0;
        }
        int copyframes = 0;
        if (resampling) {
            // We're using resampled audio!
            // Get our audio data from d->decodeaheadbuf_resampled:
            if (d->decodeaheadbuf_resampled_fillbytes <
                    (int)(d->output_channels * sizeof(DECODEMIXTYPE)))
                break;
            assert(d->decodeaheadbuf_resampled_fillbytes <=
                d->decodeaheadbuf_resampled_size);
            copyframes = (
                d->decodeaheadbuf_resampled_fillbytes /
                (d->output_channels * sizeof(DECODEMIXTYPE)));
            int fullcopyframes = copyframes;
            if (copyframes > frames - frames_written)
                copyframes = (frames - frames_written);
            assert(copyframes > 0);
            memcpy(
                output + frames_written *
                d->output_channels * sizeof(DECODEMIXTYPE),
                d->decodeaheadbuf_resampled,
                copyframes * d->output_channels * sizeof(DECODEMIXTYPE));
            frames_written += copyframes;
            if (copyframes < fullcopyframes) {
                // We only did a partial copy, cut it out of source:
                memmove(
                    d->decodeaheadbuf_resampled,
                    d->decodeaheadbuf_resampled + sizeof(DECODEMIXTYPE) *
                    d->output_channels * copyframes,
                    sizeof(DECODEMIXTYPE) *
                    d->output_channels * (fullcopyframes - copyframes));
                d->decodeaheadbuf_resampled_fillbytes -= (
                    (fullcopyframes - copyframes) *
                    sizeof(DECODEMIXTYPE) * d->output_channels);
            } else {
                // We did a full copy, wipe source.
                d->decodeaheadbuf_resampled_fillbytes = 0;
            }
        } else {
            // We're taking original unresampled audio as-is.
            // Get our audio data directly from d->decodeaheadbuf:
            if (d->decodeaheadbuf_fillbytes < (int)(
                    d->output_channels * sizeof(DECODEMIXTYPE)))
                break;
            copyframes = (
                d->decodeaheadbuf_fillbytes /
                (d->output_channels * sizeof(DECODEMIXTYPE)));
            int fullcopyframes = copyframes;
            assert(copyframes > 0);
            if (copyframes + frames_written >= frames)
                copyframes = frames - frames_written;
            int copybytes = copyframes * d->output_channels *
                sizeof(DECODEMIXTYPE);
            memcpy(
                output + frames_written *
                d->output_channels * sizeof(DECODEMIXTYPE),
                d->decodeaheadbuf, copybytes
                );
            if (copybytes < d->decodeaheadbuf_fillbytes) {
                // We did a partial copy, cut it out of the source:
                memmove(
                    d->decodeaheadbuf,
                    d->decodeaheadbuf + copybytes,
                    (d->decodeaheadbuf_fillbytes - copybytes));
                d->decodeaheadbuf_fillbytes -= copybytes;
            } else {
                // We did a full copy, wipe source.
                d->decodeaheadbuf_fillbytes = 0;
            }
            frames_written += copyframes;
        }
        if (resampling)
            d->decodeaheadbuf_resampled_fillbytes -= (
                (int)(d->output_channels * sizeof(DECODEMIXTYPE) *
                copyframes));
        else
            d->decodeaheadbuf_fillbytes -= (
                (int)(d->output_channels * sizeof(DECODEMIXTYPE) *
                copyframes));
    }

    #ifdef DEBUG_SPEW3D_AUDIO_DECODE_DATA
    // Debug print some contents:
    char *printstart = ((char *)output +
        d->decodeaheadbuf_fillbytes);
    int gotbytes = (frames_written *
        d->output_channels * sizeof(DECODEMIXTYPE));
    int printlen = gotbytes;
    if (printlen > 32) printlen = 32;
    printf(
        "spew3d_audio_decoder.c: debug: decoder "
        "addr=%p s3d_audiodecoder_Decode(): "
        "decoded %d bytes (wanted %d, "
        "d->decodeahadbuf_fillbytes %d), an excerpt: ",
        d, (int)gotbytes, (int)frames * d->output_channels *
            sizeof(DECODEMIXTYPE),
        (int)d->decodeaheadbuf_fillbytes);
    int k = 0;
    while (k < printlen) {
        uint8_t byte = *(printstart + k);
        char hexbuf[3];
        snprintf(hexbuf, sizeof(hexbuf), "%x", (int)byte);
        if (strlen(hexbuf) < 2) {
            hexbuf[2] = '\0';
            hexbuf[1] = hexbuf[0];
            hexbuf[0] = '0';
        }
        printf("%s", hexbuf);
        k++;
    }
    printf("[END]\n");
    #endif

    *out_haderror = 0;
    return frames_written;
}

void s3d_audiodecoder_ResetToStart(s3daudiodecoder *d) {
    if (d->vfserror)
        return;

    if (d->_mp3decode) {
        drmp3_uninit(d->_mp3decode);
        free(d->_mp3decode);
    }
    if (d->_wavdecode) {
        drwav_uninit(d->_wavdecode);
        free(d->_wavdecode);
    }
    if (d->_flacdecode)
        drflac_close(d->_flacdecode);
    d->decodeaheadbuf_fillbytes = 0;
    d->decodeaheadbuf_resampled_fillbytes = 0;

    if (d->vfshandle)
        if (spew3d_vfs_fseek(d->vfshandle, 0) < 0)
            d->vfserror = 1;
}

void s3d_audiodecoder_Destroy(s3daudiodecoder *d) {
    if (!d)
        return;
    if (d->vfshandle)
        spew3d_vfs_fclose(d->vfshandle);
    if (d->_mp3decode) {
        drmp3_uninit(d->_mp3decode);
        free(d->_mp3decode);
    }
    if (d->_flacdecode)
        drflac_close(d->_flacdecode);
    if (d->audiopath)
        free(d->audiopath);
    if (d->decodeaheadbuf)
        free(d->decodeaheadbuf);
    free(d);
}

int s3d_audiodecoder_HadError(s3daudiodecoder *d) {
    if (d->vfserror) {
        return 1;
    }
    return 0;
}

#endif  // SPEW3D_IMPLEMENTATION

