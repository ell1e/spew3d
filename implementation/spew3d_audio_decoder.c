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

#ifdef SPEW3D_IMPLEMENTATION

#include <assert.h>
#include <inttypes.h>
#if defined(_WIN32) || defined(_WIN64)
#include <malloc.h>
#else
#include <alloca.h>
#endif
#if defined(HAVE_SDL)
#include <SDL2/SDL.h>
#endif
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define DECODEMIXTYPE s3d_asample_t


typedef struct s3daudiodecoder {
    char *audiopath;
    int channels, input_samplerate, output_samplerate;
    int vfsflags;
    SPEW3DVFS_FILE *vfshandle;
    int vfserror;
    int output_channels;

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
    int _vorbiscachedsamplesbufsize,
        _vorbiscachedsamplesbuffill;
    char *_vorbiscachedsamplesbuf;
    int _vorbisprereadbufsize;
    char *_vorbisprereadbuf;
    stb_vorbis *_vorbisdecode;
    stb_vorbis_info _vorbisinfo;
} s3daudiodecoder;


static int s3d_audiodecoder_FillDecodeAhead(s3daudiodecoder *d);


s3daudiodecoder *audiodecoder_NewFromFile(
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
    d->output_channels = 2;
    return d;
}

int s3d_audiodecoder_GetSourceSampleRate(
        s3daudiodecoder *d
        ) {
    if (d->input_samplerate == 0 && !d->vfshandle &&
            !d->_mp3decode && !d->_wavdecode &&
            !d->_flacdecode &&
            !d->_vorbisdecode && !d->vfserror) {
        // Probably was never even touched by decoders yet.
        // Do so, so we get the basic file info:
        if (!s3d_audiodecoder_FillDecodeAhead(d))
            return 0;
    }
    return d->input_samplerate;
}

int s3d_audiodecoder_GetSourceChannels(
        s3daudiodecoder *d
        ) {
    if (d->channels == 0 && !d->vfshandle &&
            !d->_mp3decode && !d->_wavdecode &&
            !d->_flacdecode &&
            !d->_vorbisdecode && !d->vfserror) {
        // Probably was never even touched by decoders yet.
        // Do so, so we get the basic file info:
        if (!s3d_audiodecoder_FillDecodeAhead(d))
            return 0;
    }
    return d->channels;
}

int s3d_audiodecoder_SetResampleTo(
        s3daudiodecoder *d, int samplerate
        ) {
    if (samplerate < 10000 || samplerate > 100000)
        return 0;
    if (d->vfserror)
        return 0;
    d->output_samplerate = samplerate;
    return 1;
}

size_t _drmp3drwavdrflac_read_cb(
        void *ud, void *pBufferOut,
        size_t bytesToRead) {
    s3daudiodecoder *d = ud;
    if (!d->vfshandle || d->vfserror)
        return 0;
    return spew3d_vfs_fread(pBufferOut, 1, bytesToRead, d->vfshandle);
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
            !d->_flacdecode && !d->_vorbisdecode) {
        d->_wavdecode = malloc(sizeof(*d->_wavdecode));
        if (!d->_wavdecode)
            return 0;
        memset(d->_wavdecode, 0, sizeof(*d->_wavdecode));
        if (d->vfshandle) {
            if (spew3d_vfs_fseek(d->vfshandle, 0) < 0) {
                free(d->_wavdecode);
                d->_wavdecode = NULL;
                return 0;
            }
        }
        if (drwav_init(
                d->_wavdecode, _drmp3drwavdrflac_read_cb,
                _drwav_seek_cb, d, NULL
                )) {
            #if defined(DEBUG_SPEW3D_AUDIODECODE)
            printf(
                "spew3d_audio_decoder.c: debug: decoder "
                "addr=%p wav: "
                "opened for decoding: %s\n",
                d, d->audiopath
            );
            #endif
            if ((int)d->_wavdecode->channels !=
                    d->output_channels) {
                // FIXME: mono support
                drwav_uninit(d->_wavdecode);
                free(d->_wavdecode);
                d->_wavdecode = NULL;
                return 0;
            }
            d->input_samplerate = d->_wavdecode->sampleRate;
            if (d->input_samplerate < 10000 ||
                    d->input_samplerate > 100000) {
                drwav_uninit(d->_wavdecode);
                free(d->_wavdecode);
                d->_wavdecode = NULL;
                return 0;
            }
            if (d->output_samplerate == 0)
                d->output_samplerate = d->input_samplerate;
            d->channels = d->_wavdecode->channels;
        } else {
            free(d->_wavdecode);
            d->_wavdecode = NULL;
        }
    }
    if (!d->_mp3decode && !d->_wavdecode &&
            !d->_flacdecode && !d->_vorbisdecode) {
        if (d->vfshandle)
            if (spew3d_vfs_fseek(d->vfshandle, 0) < 0)
                return 0;
        if ((d->_flacdecode = drflac_open(
                _drmp3drwavdrflac_read_cb,
                _drflac_seek_cb, d, NULL
                )) != NULL) {
            #if defined(DEBUG_SPEW3D_AUDIODECODE)
            printf(
                "spew3d_audio_decoder.c: debug: decoder "
                "addr=%p flac: "
                "opened for decoding: %s\n",
                d, d->audiopath
            );
            #endif
            if (d->_flacdecode->channels !=
                    d->output_channels) {
                // FIXME: mono support
                drflac_close(d->_flacdecode);
                d->_flacdecode = NULL;
                return 0;
            }
            d->input_samplerate = d->_flacdecode->sampleRate;
            if (d->input_samplerate < 10000 ||
                    d->input_samplerate > 100000) {
                drflac_close(d->_flacdecode);
                d->_flacdecode = NULL;
                return 0;
            }
            if (d->output_samplerate == 0)
                d->output_samplerate = d->input_samplerate;
            d->channels = d->_flacdecode->channels;
        }
    }
    if (!d->_mp3decode && !d->_wavdecode &&
            !d->_flacdecode && !d->_vorbisdecode) {
        if (d->vfshandle)
            if (spew3d_vfs_fseek(d->vfshandle, 0) < 0)
                return 0;
        uint64_t fsize = 0;
        int _fserr = 0;
        if (!spew3d_vfs_Size(d->audiopath, 0, &fsize, &_fserr)) {
            d->vfserror = 1;
            return 0;
        }
        if (fsize <= 0) {
            d->vfserror = 1;
            return 0;
        }
        unsigned int input_size = 256;
        while (input_size < 1024 * 1024) {
            if (input_size > fsize)
                input_size = fsize;
            char *readbuf = malloc(input_size);
            if (!readbuf)
                return 0;
            if (spew3d_vfs_fseek(d->vfshandle, 0) < 0 ||
                    spew3d_vfs_fread(
                        readbuf, 1, input_size, d->vfshandle
                    ) < input_size) {
                d->vfserror = 1;
                free(readbuf);
                return 0;
            }
            int consumed_bytes = 0;
            int pushdata_error = 0;
            assert(input_size > 0);
            d->_vorbisdecode = stb_vorbis_open_pushdata(
                (unsigned char*)readbuf, input_size,
                &consumed_bytes, &pushdata_error,
                NULL
            );
            free(readbuf);
            if (!d->_vorbisdecode) {
                if (pushdata_error != VORBIS_need_more_data ||
                        input_size >= fsize)
                    break;
                input_size *= 2; // try again with more data
                continue;
            }
            if (spew3d_vfs_fseek(d->vfshandle, consumed_bytes) < 0) {
                d->vfserror = 1;
                stb_vorbis_close(d->_vorbisdecode);
                d->_vorbisdecode = NULL;
                return 0;
            }
            d->_vorbisinfo = stb_vorbis_get_info(
                d->_vorbisdecode);
            #if defined(DEBUG_SPEW3D_AUDIODECODE)
            printf(
                "spew3d_audio_decoder.c: debug: decoder "
                "addr=%p ogg: "
                "opened for decoding: %s\n",
                d, d->audiopath
            );
            #endif
            if (d->_vorbisinfo.channels != d->output_channels) {
                // FIXME: mono support
                stb_vorbis_close(d->_vorbisdecode);
                d->_vorbisdecode = NULL;
                return 0;
            }
            d->input_samplerate = d->_vorbisinfo.sample_rate;
            if (d->input_samplerate < 10000 ||
                    d->input_samplerate > 100000) {
                stb_vorbis_close(d->_vorbisdecode);
                d->_vorbisdecode = NULL;
                return 0;
            }
            if (d->output_samplerate == 0)
                d->output_samplerate = d->input_samplerate;
            d->channels = d->_vorbisinfo.channels;
            break;
        }
    }
    if (!d->_mp3decode && !d->_wavdecode &&
            !d->_flacdecode && !d->_vorbisdecode) {
        d->_mp3decode = malloc(sizeof(*d->_mp3decode));
        if (!d->_mp3decode)
            return 0;
        memset(d->_mp3decode, 0, sizeof(*d->_mp3decode));
        if (d->vfshandle) {
            if (spew3d_vfs_fseek(d->vfshandle, 0) < 0) {
                free(d->_mp3decode);
                d->_mp3decode = NULL;
                return 0;
            }
        }
        if (drmp3_init(
                d->_mp3decode, _drmp3drwavdrflac_read_cb,
                _drmp3_seek_cb, d, NULL
                )) {
            #if defined(DEBUG_SPEW3D_AUDIODECODE)
            printf(
                "spew3d_audio_decoder.c: debug: decoder "
                "addr=%p mp3: "
                "opened for decoding: %s\n",
                d, d->audiopath
            );
            #endif
            if ((int)d->_mp3decode->channels !=
                    d->output_channels) {
                // FIXME: mono support
                drmp3_uninit(d->_mp3decode);
                free(d->_mp3decode);
                d->_mp3decode = NULL;
                return 0;
            }
            d->input_samplerate = d->_mp3decode->sampleRate;
            if (d->input_samplerate < 10000 ||
                    d->input_samplerate > 100000) {
                drmp3_uninit(d->_mp3decode);
                free(d->_mp3decode);
                d->_mp3decode = NULL;
                return 0;
            }
            if (d->output_samplerate == 0)
                d->output_samplerate = d->input_samplerate;
            d->channels = d->_mp3decode->channels;
        } else {
            free(d->_mp3decode);
            d->_mp3decode = NULL;
        }
    }
    if (!d->_mp3decode && !d->_wavdecode &&
            !d->_flacdecode && !d->_vorbisdecode) {
        d->vfserror = 1;
        return 0;
    }
    if (!d->decodeaheadbuf) {
        assert(d->input_samplerate > 0);
        assert(d->channels > 0);
        d->decodeaheadbuf = malloc(
            d->input_samplerate * sizeof(DECODEMIXTYPE) * d->channels
        );
        if (!d->decodeaheadbuf) {
            d->decodeaheadbuf_fillbytes = 0;
            return 0;
        }
    }
    int want_to_read_bytes = (
        d->input_samplerate * sizeof(DECODEMIXTYPE) * d->channels
    ) - d->decodeaheadbuf_fillbytes;
    int want_to_read_frames = (
        want_to_read_bytes / (sizeof(DECODEMIXTYPE) * d->channels)
    );
    if (want_to_read_frames <= 0) {
        return 1;
    }
    uint64_t read_frames = 0;
    if (sizeof(DECODEMIXTYPE) == sizeof(int16_t) && d->_mp3decode) {
        assert(
            (int)want_to_read_bytes +
                (int)d->decodeaheadbuf_fillbytes <=
            (int)d->input_samplerate * (int)sizeof(DECODEMIXTYPE) *
                (int)d->channels
        );
        assert(want_to_read_frames > 0);
        assert(
            (int)want_to_read_frames * (int)sizeof(DECODEMIXTYPE) *
                (int)d->channels +
                d->decodeaheadbuf_fillbytes <=
            (int)d->input_samplerate * (int)sizeof(DECODEMIXTYPE) *
                (int)d->channels
        );
        read_frames = drmp3_read_pcm_frames_s16(
            d->_mp3decode, want_to_read_frames,
            (drmp3_int16 *)((char *)d->decodeaheadbuf +
            d->decodeaheadbuf_fillbytes)
        );

        #if defined(DEBUG_SPEW3D_AUDIODECODE)
        printf(
            "spew3d_audio_decoder.c: debug: decoder "
            "addr=%p mp3: "
            "frames=%d(%dB) fillbytes(after)=%d/%d\n",
            d, (int)read_frames,
            (int)(read_frames * sizeof(DECODEMIXTYPE) * d->channels),
            (int)(d->decodeaheadbuf_fillbytes +
            read_frames * sizeof(DECODEMIXTYPE) * d->channels),
            (int)(d->input_samplerate * sizeof(DECODEMIXTYPE) *
            d->channels)
        );
        #endif
    } else if (sizeof(DECODEMIXTYPE) == sizeof(int16_t) &&
            d->_wavdecode) {
        read_frames = drwav_read_pcm_frames_s16(
            d->_wavdecode, want_to_read_frames,
            (drwav_int16 *)((char *)d->decodeaheadbuf +
            d->decodeaheadbuf_fillbytes)
        );

        #if defined(DEBUG_SPEW3D_AUDIODECODE)
        printf(
            "spew3d_audio_decoder.c: debug: decoder "
            "addr=%p wav: "
            "frames=%d(%dB) fillbytes(after)=%d/%d\n",
            d, (int)read_frames,
            (int)(read_frames * sizeof(DECODEMIXTYPE) * d->channels),
            (int)(d->decodeaheadbuf_fillbytes +
            read_frames * sizeof(DECODEMIXTYPE) * d->channels),
            (int)(d->input_samplerate * sizeof(DECODEMIXTYPE) *
            d->channels)
        );
        #endif
    } else if (sizeof(DECODEMIXTYPE) == sizeof(int16_t) &&
            d->_flacdecode) {
        read_frames = drflac_read_pcm_frames_s16(
            d->_flacdecode, want_to_read_frames,
            (drflac_int16 *)((char *)d->decodeaheadbuf +
            d->decodeaheadbuf_fillbytes)
        );

        #if defined(DEBUG_SPEW3D_AUDIODECODE)
        printf(
            "spew3d_audio_decoder.c: debug: decoder "
            "addr=%p flac: "
            "frames=%d(%dB) fillbytes(after)=%d/%d\n",
            d, (int)read_frames,
            (int)(read_frames * sizeof(DECODEMIXTYPE) * d->channels),
            (int)(d->decodeaheadbuf_fillbytes +
            read_frames * sizeof(DECODEMIXTYPE) * d->channels),
            (int)(d->input_samplerate * sizeof(DECODEMIXTYPE) *
            d->channels)
        );
        #endif
    } else if (sizeof(DECODEMIXTYPE) == sizeof(int16_t) &&
            d->_vorbisdecode) {
        read_frames = 0;
        assert(d->channels == 2);  // FIXME: change for mono
        DECODEMIXTYPE *writeto = (DECODEMIXTYPE *)(
            ((char *)d->decodeaheadbuf +
             d->decodeaheadbuf_fillbytes)
        );
        while (read_frames < (uint64_t)want_to_read_frames &&
                d->_vorbiscachedsamplesbuffill >=
                (int)sizeof(DECODEMIXTYPE) * (int)d->channels) {
            memcpy(
                writeto, d->_vorbiscachedsamplesbuf,
                sizeof(DECODEMIXTYPE) * d->channels
            );
            writeto += d->channels;
            d->_vorbiscachedsamplesbuffill -= (
                sizeof(DECODEMIXTYPE) * d->channels
            );
            if (d->_vorbiscachedsamplesbuffill > 0)
                memmove(
                    d->_vorbiscachedsamplesbuf,
                    ((char *)d->_vorbiscachedsamplesbuf) +
                        sizeof(DECODEMIXTYPE) * d->channels,
                    d->_vorbiscachedsamplesbuffill
                );
            read_frames++;
            assert(
                (char *)d->decodeaheadbuf +
                read_frames * sizeof(DECODEMIXTYPE) * d->channels +
                d->decodeaheadbuf_fillbytes == (char *)writeto
            );
        }
        int input_size = d->_vorbisprereadbufsize;
        if (input_size < 1024)
            input_size = 1024;
        while (read_frames < (uint64_t)want_to_read_frames) {
            assert(
                (char *)d->decodeaheadbuf +
                read_frames * sizeof(DECODEMIXTYPE) * d->channels +
                d->decodeaheadbuf_fillbytes == (char *)writeto
            );
            if (input_size > 1024 * 10) {
                #if defined(DEBUG_SPEW3D_AUDIODECODE)
                printf(
                    "spew3d_audio_decoder.c: warning: decoder "
                    "addr=%p ogg: "
                    "couldn't read next packet even with "
                    "pushdata size %d\n",
                    d, (int)(input_size / 2)
                );
                #endif
                // Ok, this is unreasonable. Assume buggy file.
                goto vorbisfilefail;
            }
            char *readbuf = d->_vorbisprereadbuf;
            if (!readbuf || d->_vorbisprereadbufsize != input_size) {
                readbuf = malloc(input_size);
                if (!readbuf)
                    return 0;
                if (d->_vorbisprereadbuf)
                    free(d->_vorbisprereadbuf);
                d->_vorbisprereadbuf = readbuf;
                d->_vorbisprereadbufsize = input_size;
            }
            int64_t offset = spew3d_vfs_ftell(d->vfshandle);
            if (offset < 0)
                goto vorbisfilefail;
            int result = spew3d_vfs_fread(readbuf, 1,
                input_size, d->vfshandle);
            if ((result <= 0 || result < input_size) &&
                    !spew3d_vfs_feof(d->vfshandle)) {
                vorbisfilefail:
                stb_vorbis_close(d->_vorbisdecode);
                d->_vorbisdecode = NULL;
                d->vfserror = 1;
                if (d->_vorbisprereadbuf)
                    free(d->_vorbisprereadbuf);
                d->_vorbisprereadbuf = NULL;
                d->_vorbisprereadbufsize = 0;
                return 0;
            } else if (result <= 0) {
                assert(spew3d_vfs_feof(d->vfshandle));
                break;
            }
            if (spew3d_vfs_fseek(d->vfshandle, offset) < 0)
                goto vorbisfilefail;
            int channels_found = 0;
            int samples_found = 0;
            float **outputs = NULL;
            stb_vorbis_get_error(d->_vorbisdecode);  // clear error
            int bytes_used = stb_vorbis_decode_frame_pushdata(
                d->_vorbisdecode, (unsigned char*)readbuf, result,
                (int *)&channels_found, &outputs,
                (int *)&samples_found
            );
            int pushdata_error = (
                stb_vorbis_get_error(d->_vorbisdecode)
            );
            if (bytes_used == 0 && samples_found == 0) {
                if (pushdata_error != VORBIS_need_more_data) {
                    #if defined(DEBUG_SPEW3D_AUDIODECODE)
                    printf(
                        "spew3d_audio_decoder.c: warning: decoder "
                        "addr=%p ogg: "
                        "failed with pushdata error: %d\n",
                        d, pushdata_error
                    );
                    #endif
                    stb_vorbis_close(d->_vorbisdecode);
                    d->_vorbisdecode = NULL;
                    d->vfserror = 1;
                    free(readbuf);
                    return 0;
                }
                if (result < input_size && spew3d_vfs_feof(d->vfshandle)) {
                    #if defined(DEBUG_SPEW3D_AUDIODECODE)
                    printf(
                        "spew3d_audio_decoder.c: debug: decoder "
                        "addr=%p ogg: "
                        "end of file\n", d
                    );
                    #endif
                    break;  // hit the maximum block already
                }
                input_size *= 2; // try again with more data
                continue;
            }
            if (samples_found == 0 && bytes_used > 0) {
                // Keep reading as per stb_vorbis documentation.
                // (Block that didn't generate data, apparently can happen)
                continue;
            }
            if (spew3d_vfs_fseek(d->vfshandle, offset +
                    (int64_t)bytes_used) < 0)
                goto vorbisfilefail;
            if (channels_found != d->channels)
                goto vorbisfilefail;
            DECODEMIXTYPE *channelbuf = alloca(
                sizeof(*channelbuf) * channels_found
            );
            if (!channelbuf)
                goto vorbisfilefail;
            unsigned int i = 0;
            while (i < (unsigned int)samples_found) {
                unsigned int k = 0;
                while (k < (unsigned int)d->channels) {
                    int64_t value = (outputs[k][i] *
                        (((double)S3D_ASAMPLE_MAX) + 1.0));
                    if (value > (int64_t)S3D_ASAMPLE_MAX)
                        value = (int64_t)S3D_ASAMPLE_MAX;
                    if (value < (int64_t)S3D_ASAMPLE_MIN)
                        value = (int64_t)S3D_ASAMPLE_MIN;
                    channelbuf[k] = value;
                    k++;
                }
                k = 0;
                while (k < (unsigned int)d->channels) {
                    if (read_frames < (uint64_t)want_to_read_frames) {
                        assert(
                            ((char *)writeto) <
                            (char *)d->decodeaheadbuf +
                            (d->input_samplerate * sizeof(DECODEMIXTYPE) *
                             d->channels)
                        );
                        *writeto = channelbuf[k];
                        writeto++;
                        k++;
                        continue;
                    }
                    int newfill = d->_vorbiscachedsamplesbuffill +
                        sizeof(DECODEMIXTYPE);
                    if (newfill >
                            d->_vorbiscachedsamplesbufsize) {
                        char *newbuf = realloc(
                            d->_vorbiscachedsamplesbuf,
                            newfill
                        );
                        if (!newbuf)
                            goto vorbisfilefail;
                        d->_vorbiscachedsamplesbuf = newbuf;
                        d->_vorbiscachedsamplesbufsize = newfill;
                    }
                    DECODEMIXTYPE *bufptr = (DECODEMIXTYPE *)(
                        (char*)d->_vorbiscachedsamplesbuf +
                        d->_vorbiscachedsamplesbuffill
                    );
                    d->_vorbiscachedsamplesbuffill = newfill;
                    *bufptr = channelbuf[k];
                    k++;
                }
                if (read_frames < (uint64_t)want_to_read_frames)
                    read_frames++;

                assert(
                    (char *)d->decodeaheadbuf +
                    read_frames * sizeof(DECODEMIXTYPE) * d->channels +
                    d->decodeaheadbuf_fillbytes == (char *)writeto
                );

                i++;
            }
        }
        assert((
            (char *)d->decodeaheadbuf +
            read_frames * sizeof(DECODEMIXTYPE) * d->channels +
            d->decodeaheadbuf_fillbytes == (char *)writeto
        ) && (read_frames == (uint64_t)want_to_read_frames ||
              spew3d_vfs_feof(d->vfshandle)));

        #if defined(DEBUG_SPEW3D_AUDIODECODE)
        printf(
            "spew3d_audio_decoder.c: debug: "
            "decoder addr=%p ogg: "
            "frames=%d(%dB) fillbytes(after)=%d/%d\n",
            d, (int)read_frames,
            (int)(read_frames * sizeof(DECODEMIXTYPE) * d->channels),
            (int)(d->decodeaheadbuf_fillbytes +
            read_frames * sizeof(DECODEMIXTYPE) * d->channels),
            (int)(d->input_samplerate * sizeof(DECODEMIXTYPE) *
            d->channels)
        );
        #endif
    } else {
        #if defined(DEBUG_SPEW3D_AUDIODECODE)
        fprintf(stderr, "spew3d_audio_decoder.c: warning: "
            "unknown decode type");
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
        read_frames * sizeof(DECODEMIXTYPE) * d->channels
    );
    return 1;
}

static int s3d_audiodecoder_FillDecodeAheadResampled(
        s3daudiodecoder *d
        ) {
    int frames_written = 0;
    if (d->vfserror || !s3d_audiodecoder_FillDecodeAhead(d)) {
        return 0;
    }
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
        assert(sizeof(DECODEMIXTYPE) == 2);
        if (!d->decodeaheadbuf_resampled) {
            int buf_size = (
                d->output_samplerate * sizeof(DECODEMIXTYPE) *
                d->output_channels * 2
            );
            assert(buf_size > 0);
            while (buf_size < (int)(d->input_samplerate *
                    sizeof(DECODEMIXTYPE) *
                    d->output_channels))
                buf_size *= 2;
            d->decodeaheadbuf_resampled_size = buf_size;
            d->decodeaheadbuf_resampled = (
                malloc(d->decodeaheadbuf_resampled_size)
            );
            if (!d->decodeaheadbuf_resampled) {
                d->vfserror = 1;
                return 0;
            }
        }

        while (d->decodeaheadbuf_resampled_fillbytes <
                want_resampled_buffered_bytes) {
            int unresampled_frames = (
                d->decodeaheadbuf_fillbytes / (sizeof(DECODEMIXTYPE) *
                d->output_channels)
            ) * d->resample_factor;
            assert(unresampled_frames >= 0);
            if (unresampled_frames == 0 && (
                    d->decodeaheadbuf_fillbytes /
                    (sizeof(DECODEMIXTYPE) *
                    d->output_channels)) > 0)
                unresampled_frames = 1;
            int unresampled_bytes = (
                unresampled_frames * sizeof(DECODEMIXTYPE) *
                d->output_channels
            );
            if (unresampled_bytes <= 0)
                break;

            #ifndef SPEW3D_OPTION_DISABLE_SDL
            SDL_AudioCVT cvt;
            memset(&cvt, 0, sizeof(cvt));
            SDL_BuildAudioCVT(
                &cvt, AUDIO_S16, d->output_channels,
                d->input_samplerate,
                AUDIO_S16, d->output_channels,
                d->output_samplerate
            );
            if (cvt.len * cvt.len_mult > (
                    d->decodeaheadbuf_resampled_size -
                    d->decodeaheadbuf_resampled_fillbytes))
                break;
            cvt.len = unresampled_bytes;
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
                    d->input_samplerate * sizeof(DECODEMIXTYPE) *
                    d->output_channels - cvt.len
                );
            }
            assert(cvt.len_cvt > 0);
            d->decodeaheadbuf_resampled_fillbytes += cvt.len_cvt;
            #else
            fprintf(  // FIXME !!!
                stderr, "spew3d_audio_decoder.c: "
                "resampling code path not implemented");
            _exit(1);
            #endif
        }
    }
    return 1;
}

int s3d_audiodecoder_Decode(
        s3daudiodecoder *d, char *output, int frames,
        int *out_haderror
        ) {
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
            if (d->decodeaheadbuf_resampled_fillbytes <
                    (int)(d->output_channels * sizeof(DECODEMIXTYPE)))
                break;
            copyframes = (
                d->decodeaheadbuf_resampled_fillbytes /
                (d->output_channels * sizeof(DECODEMIXTYPE)));
            int fullcopyframes = copyframes;
            assert(copyframes > 0);
            memcpy(
                output + frames_written *
                d->output_channels * sizeof(DECODEMIXTYPE),
                d->decodeaheadbuf_resampled,
                copyframes * d->output_channels * sizeof(DECODEMIXTYPE));
            frames_written += copyframes;
            if (copyframes < fullcopyframes)
                memmove(
                    d->decodeaheadbuf_resampled,
                    d->decodeaheadbuf_resampled + sizeof(DECODEMIXTYPE) *
                    d->output_channels * copyframes,
                    sizeof(DECODEMIXTYPE) *
                    d->output_channels * (fullcopyframes - copyframes));
        } else {
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
            memcpy(
                output + frames_written *
                d->output_channels * sizeof(DECODEMIXTYPE),
                d->decodeaheadbuf,
                copyframes * d->output_channels * sizeof(DECODEMIXTYPE));
            if (copyframes < fullcopyframes)
                memmove(
                    d->decodeaheadbuf,
                    d->decodeaheadbuf + sizeof(DECODEMIXTYPE) *
                    d->output_channels * copyframes,
                    sizeof(DECODEMIXTYPE) *
                    d->output_channels * (fullcopyframes - copyframes));
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
    if (d->_vorbisdecode)
        stb_vorbis_close(d->_vorbisdecode);
    d->decodeaheadbuf_fillbytes = 0;
    d->decodeaheadbuf_resampled_fillbytes = 0;
    d->_vorbiscachedsamplesbufsize = 0;
    d->_vorbiscachedsamplesbuffill = 0;
    free(d->_vorbiscachedsamplesbuf);
    d->_vorbiscachedsamplesbuf = NULL;
    d->_vorbisprereadbufsize = 0;
    free(d->_vorbisprereadbuf);
    d->_vorbisprereadbuf = NULL;

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
    if (d->_vorbisdecode)
        stb_vorbis_close(d->_vorbisdecode);
    if (d->_vorbisprereadbuf)
        free(d->_vorbisprereadbuf);
    if (d->_vorbiscachedsamplesbuf)
        free(d->_vorbiscachedsamplesbuf);
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

