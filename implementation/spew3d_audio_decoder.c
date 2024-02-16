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

typedef struct s3d_audio_decoder {
    char *audiopath;
    int input_channels, input_samplerate,
        output_samplerate, output_channels;
    int vfsflags;
    SPEW3DVFS_FILE *vfshandle;
    int vfserror;

    int decode_endoffile;
    char *decodeaheadbuf;
    int decodeaheadbuf_size;
    int decodeaheadbuf_fillbytes;
    s3d_audio_resampler *resampler;

    drmp3 *_mp3decode;
    drwav *_wavdecode;
    drflac *_flacdecode;
} s3d_audio_decoder;

static int s3d_audiodecoder_FillDecodeAhead(s3d_audio_decoder *d);

static int s3d_audiodecoder_FillDecodeAheadResampled(s3d_audio_decoder *d);

S3DEXP s3d_audio_decoder *audiodecoder_NewFromFile(
        const char *filepath
        ) {
    return audiodecoder_NewFromFileEx(filepath, 0);
}

S3DEXP s3d_audio_decoder *audiodecoder_NewFromFileEx(
        const char *filepath, int vfsflags
        ) {
    int _exists = 0;
    int _existsfserr = 0;
    if (!spew3d_vfs_Exists(filepath, vfsflags,
            &_exists, &_existsfserr) || !_exists) {
        return NULL;
    }
    s3d_audio_decoder *d = malloc(sizeof(*d));
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

S3DEXP int s3d_audiodecoder_GetSourceSampleRate(
        s3d_audio_decoder *d
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

S3DEXP int s3d_audiodecoder_GetOutputChannels(
        s3d_audio_decoder *d
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

S3DEXP int s3d_audiodecoder_GetSourceChannels(
        s3d_audio_decoder *d
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

S3DEXP int s3d_audiodecoder_SetChannelAdjustTo(
        s3d_audio_decoder *d, int channels
        ) {
    if (channels < 1 || channels > 10)
        return 0;
    if (d->resampler) {
        s3d_audioresampler_Free(d->resampler);
        d->resampler = NULL;
    }
    d->output_channels = channels;
    return 1;
}

S3DEXP int s3d_audiodecoder_SetResampleTo(
        s3d_audio_decoder *d, int samplerate
        ) {
    if (samplerate < 10000 || samplerate > 100000)
        return 0;
    if (d->resampler) {
        s3d_audioresampler_SetNewTargetSamplerate(
            d->resampler, samplerate);
    }
    d->output_samplerate = samplerate;
    return 1;
}

S3DHID size_t _drmp3drwavdrflac_read_cb(
        void *ud, void *pBufferOut,
        size_t bytesToRead) {
    s3d_audio_decoder *d = ud;
    if (!d->vfshandle || d->vfserror)
        return 0;
    size_t result = spew3d_vfs_fread(
        pBufferOut, 1, bytesToRead, d->vfshandle
    );
    if (result == 0 && spew3d_vfs_ferror(d->vfshandle))
        d->vfserror = 1;
    return result;
}

S3DHID uint32_t _drmp3_seek_cb(void *ud,
        int offset, drmp3_seek_origin origin) {
    s3d_audio_decoder *d = ud;
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

S3DHID uint32_t _drwav_seek_cb(void *ud,
        int offset, drwav_seek_origin origin) {
    s3d_audio_decoder *d = ud;
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

S3DHID uint32_t _drflac_seek_cb(void *ud,
        int offset, drflac_seek_origin origin) {
    s3d_audio_decoder *d = ud;
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

S3DHID static void _expand_s16_to_s32(char *buf, uint64_t samples) {
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

S3DHID static int s3d_audiodecoder_FillDecodeAhead(
        s3d_audio_decoder *d
        ) {
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
        int wantsize = (
            d->input_samplerate * sizeof(DECODEMIXTYPE) *
                d->input_channels);
        d->decodeaheadbuf = malloc(wantsize);
        if (!d->decodeaheadbuf) {
            d->decodeaheadbuf_fillbytes = 0;
            d->vfserror = 1;
            return 0;
        }
        d->decodeaheadbuf_size = wantsize;
    }
    int want_to_read_bytes = (
        d->decodeaheadbuf_size - d->decodeaheadbuf_fillbytes
    );
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
            (int)d->decodeaheadbuf_size
        );
        assert(want_to_read_frames > 0);
        assert(
            (int)want_to_read_frames * (int)sizeof(DECODEMIXTYPE) *
                (int)d->input_channels +
                d->decodeaheadbuf_fillbytes <=
            (int)d->decodeaheadbuf_size
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
        assert((int)(d->decodeaheadbuf_fillbytes +
            read_frames * sizeof(DECODEMIXTYPE) *
            d->input_channels) <= d->decodeaheadbuf_size);

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
            (int)(d->decodeaheadbuf_size)
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
    assert(d->decodeaheadbuf_fillbytes >= 0);
    return 1;
}

S3DHID static int s3d_audiodecoder_FillDecodeAheadResampled(
        s3d_audio_decoder *d) {
    if (d->vfserror ||
            !s3d_audiodecoder_FillDecodeAhead(d))
        return 0;
    if (d->resampler == NULL) {
        d->resampler = s3daudioresampler_New(
            &d->decodeaheadbuf, &d->decodeaheadbuf_size,
            &d->decodeaheadbuf_fillbytes,
            d->input_samplerate, d->output_samplerate,
            d->output_channels);
        if (!d->resampler) {
            d->vfserror = 1;
            return 0;
        }
    }
    if (!s3d_audioresampler_FillBufResampled(d->resampler)) {
        d->vfserror = 1;
        return 0;
    }
    return 1;
}

S3DEXP int s3d_audiodecoder_Decode(
        s3d_audio_decoder *d, char *output, int frames,
        int *out_haderror
        ) {
    // Ensure basic data is set on our source stream:
    if (d->input_samplerate == 0) {
        if (d->vfserror ||
                !s3d_audiodecoder_FillDecodeAhead(d) ||
                d->input_samplerate == 0) {
            // We failed to obtain basic data.
            *out_haderror = 1;
            return 0;
        }
    }
    if (frames <= 0) {
        *out_haderror = 0;
        return 0;
    }

    // Determine what we want to do:
    const int resampling = (d->input_samplerate !=
        d->output_samplerate);
    int frames_written = 0;

    while (frames_written < frames) {
        if (d->vfserror || (resampling &&
                !s3d_audiodecoder_FillDecodeAheadResampled(d)) ||
                (!resampling &&
                !s3d_audiodecoder_FillDecodeAhead(d))) {
            *out_haderror = 1;
            return 0;
        }
        int copyframes = 0;
        if (resampling) {
            // We're using resampled audio!
            // Get our audio data from res->resampledbuf:
            s3d_audio_resampler *res = d->resampler;
            assert(res != NULL);
            if (res->resampledbuf_fillbytes <
                    (int)(d->output_channels * sizeof(DECODEMIXTYPE))) {
                if (frames_written == 0)
                    d->decode_endoffile = 1;
                break;
            }
            assert(res->resampledbuf_fillbytes <=
                res->resampledbuf_size);
            copyframes = (
                res->resampledbuf_fillbytes /
                (d->output_channels * sizeof(DECODEMIXTYPE)));
            int fullcopyframes = copyframes;
            if (copyframes > frames - frames_written)
                copyframes = (frames - frames_written);
            int copybytes = copyframes * sizeof(DECODEMIXTYPE) *
                d->output_channels;
            assert(copyframes > 0);
            memcpy(
                output + frames_written *
                d->output_channels * sizeof(DECODEMIXTYPE),
                res->resampledbuf,
                copybytes);
            frames_written += copyframes;
            if (copybytes < res->resampledbuf_fillbytes) {
                // We only did a partial copy, cut it out of source:
                memmove(
                    res->resampledbuf,
                    res->resampledbuf + copybytes,
                    res->resampledbuf_fillbytes - copybytes);
                res->resampledbuf_fillbytes -= copybytes;
            } else {
                // We did a full copy, wipe source.
                res->resampledbuf_fillbytes = 0;
            }
        } else {
            // We're taking original unresampled audio as-is.
            // Get our audio data directly from d->decodeaheadbuf:
            if (d->decodeaheadbuf_fillbytes < (int)(
                    d->output_channels * sizeof(DECODEMIXTYPE))) {
                if (frames_written == 0)
                    d->decode_endoffile = 1;
                break;
            }
            copyframes = (
                d->decodeaheadbuf_fillbytes /
                (d->output_channels * sizeof(DECODEMIXTYPE)));
            int fullcopyframes = copyframes;
            assert(copyframes > 0);
            if (copyframes + frames_written >= frames)
                copyframes = frames - frames_written;
            assert(copyframes > 0);
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

S3DEXP void s3d_audiodecoder_ResetToStart(s3d_audio_decoder *d) {
    if (d->vfserror)
        return;

    if (d->_mp3decode != NULL) {
        drmp3_uninit(d->_mp3decode);
        free(d->_mp3decode);
    }
    if (d->_wavdecode != NULL) {
        drwav_uninit(d->_wavdecode);
        free(d->_wavdecode);
    }
    if (d->_flacdecode != NULL) {
        drflac_close(d->_flacdecode);
    }
    d->decodeaheadbuf_fillbytes = 0;
    if (d->resampler != NULL) {
        d->resampler->resampledbuf_fillbytes = 0;
    }

    if (d->vfshandle != NULL) {
        if (spew3d_vfs_fseek(d->vfshandle, 0) < 0)
            d->vfserror = 1;
    }
    d->decode_endoffile = 0;
}

S3DEXP void s3d_audiodecoder_Destroy(s3d_audio_decoder *d) {
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
    if (d->resampler) {
        s3d_audioresampler_Free(d->resampler);
    }
    free(d);
}

S3DEXP int s3d_audiodecoder_HadError(s3d_audio_decoder *d) {
    if (d->vfserror) {
        return 1;
    }
    return 0;
}

#endif  // SPEW3D_IMPLEMENTATION

