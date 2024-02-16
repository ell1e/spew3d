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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#ifndef SPEW3D_OPTION_DISABLE_SDL
#include <SDL2/SDL.h>
#endif

static s3d_audio_sink **open_sink = NULL;
static volatile int32_t open_sink_count = 0;
static s3d_mutex *sink_list_mutex = NULL;
static char **sink_sdl_soundcards_list = NULL;
static int64_t sink_sdl_soundcards_refresh_ts = 0;
static char **
    _internal_spew3d_audio_sink_GetSoundcardListOutput(int type);
static void _audiocb_SDL2(void *udata, uint8_t *stream, int len);
S3DHID void _internal_spew3d_audio_sink_ProcessInput();
S3DHID void _internal_spew3d_audio_sink_MarkPermaDestroyedUnchecked(
    s3d_audio_sink *sink
);

typedef struct s3d_audio_sink_internal {
    int refcount, wasclosed;
    int ringbufferplaynum, ringbufferfillnum;

    int sinksrc_type;
    union {
        struct {
            s3d_audio_decoder *sinksrc_decoder;
        };
        struct {
            s3d_audio_mixer *sinksrc_mixer;
        };
    };

    union {
        #ifndef SPEW3D_OPTION_DISABLE_SDL
        int output_sdlaudiodevice_opened,
            output_sdlaudiodevice_failed;
        SDL_AudioDeviceID output_sdlaudiodevice;
        #endif
    };
} s3d_audio_sink_internal;

#define SINKIDATA(sink) ((s3d_audio_sink_internal *)\
    ((s3d_audio_sink *)sink)->internalptr)

#define SINKSRC_NONE 0
#define SINKSRC_SINK 1
#define SINKSRC_DECODER 2
#define SINKSRC_MIXER 3

static void __attribute__((constructor)) _spew3d_audio_sink_dllinit() {
    if (sink_list_mutex != NULL)
        return;
    sink_list_mutex = mutex_Create();
    if (!sink_list_mutex) {
        fprintf(
            stderr, "spew3d_audio_sink.c: error: "
            "mutex_Create() failed for sink_list_mutex"
        );
        _exit(1);
        return;
    }
}

S3DEXP int spew3d_audio_sink_FeedFromDecoder(
        s3d_audio_sink *sink, s3d_audio_decoder *decoder
        ) {
    assert(sink != NULL);
    assert(!SINKIDATA(sink)->wasclosed);
    assert(decoder != NULL);
    if (!s3d_audiodecoder_SetChannelAdjustTo(
            decoder, sink->channels
            ) || !s3d_audiodecoder_SetResampleTo(
            decoder, sink->samplerate
            ))
        return 0;
    SINKIDATA(sink)->sinksrc_decoder = decoder;
    SINKIDATA(sink)->sinksrc_type = SINKSRC_DECODER;
    #if defined(DEBUG_SPEW3D_AUDIO_SINK)
    printf(
        "spew3d_audio_sink.c: debug: sink "
        "addr=%p: spew3d_audio_sink_FeedFromDecoder() "
        "to decoder: addr=%p\n",
        sink, decoder
    );
    #endif
    return 1;
}

S3DEXP s3d_audio_mixer *spew3d_audio_sink_GetMixer(
        s3d_audio_sink *sink
        ) {
    mutex_Lock(sink_list_mutex);
    if (SINKIDATA(sink)->sinksrc_type != SINKSRC_NONE) {
        if (SINKIDATA(sink)->sinksrc_type == SINKSRC_MIXER) {
            s3d_audio_mixer *mixer =
                SINKIDATA(sink)->sinksrc_mixer;
            mutex_Release(sink_list_mutex);
            return mixer;
        }
        mutex_Release(sink_list_mutex);
        return NULL;
    }
    #if defined(DEBUG_SPEW3D_AUDIO_MIXER)
    printf(
        "spew3d_audio_sinker.c: debug: sink "
        "addr=%p: spew3d_audio_sink_GetMixer() "
        "called, getting new mixer...\n",
        sink
    );
    #endif
    s3d_audio_mixer *mixer = spew3d_audio_mixer_New(
        sink->samplerate, sink->channels
    );
    if (!mixer) {
        mutex_Release(sink_list_mutex);
        return NULL;
    }
    SINKIDATA(sink)->sinksrc_type = SINKSRC_MIXER;
    SINKIDATA(sink)->sinksrc_mixer = mixer;
    mutex_Release(sink_list_mutex);
    return mixer;
}

S3DHID void _internal_spew3d_audio_sink_ProcessInput(
        s3d_audio_sink *sink
        ) {
    assert(sink != NULL);
    assert(!SINKIDATA(sink)->wasclosed);
    const int channels = sink->channels;
    int nocopyneeded = 0;
    int didcopy = 0;
    while (1) {
        int nextnum = SINKIDATA(sink)->ringbufferfillnum + 1;
        nextnum = (nextnum % sink->ringbuffersegmentcount);
        if (nextnum == SINKIDATA(sink)->ringbufferplaynum) {
            nocopyneeded = 1;
            break;
        }
        /*printf("sink input: "
            "sink %p fill %d play %d next %d segments %d\n", sink,
            (int)SINKIDATA(sink)->ringbufferfillnum,
            (int)SINKIDATA(sink)->ringbufferplaynum,
            nextnum, (int)sink->ringbuffersegmentcount);*/
        if (SINKIDATA(sink)->sinksrc_type == SINKSRC_DECODER) {
            assert(SINKIDATA(sink)->sinksrc_decoder != NULL);
            assert(SINKIDATA(sink)->sinksrc_decoder->output_channels ==
                channels);
            int framecount = ((SPEW3D_SINK_AUDIOBUF_BYTES *
                channels) / (sizeof(s3d_asample_t) * 2));
            assert(framecount > 0);
            int haderror = 0;
            int result = s3d_audiodecoder_Decode(
                SINKIDATA(sink)->sinksrc_decoder,
                sink->ringbuffer +
                nextnum * SPEW3D_SINK_AUDIOBUF_BYTES * channels,
                framecount, &haderror
            );
            if (result < framecount)
                memset(sink->ringbuffer +
                    nextnum * SPEW3D_SINK_AUDIOBUF_BYTES * channels +
                    result * (channels * sizeof(s3d_asample_t)),
                    0,
                    (framecount - result) * (2 * sizeof(s3d_asample_t)));
            SINKIDATA(sink)->ringbufferfillnum = nextnum;
            didcopy = 1;
        } else if (SINKIDATA(sink)->sinksrc_type == SINKSRC_MIXER) {
            assert(SINKIDATA(sink)->sinksrc_mixer != NULL);
            int framecount = ((SPEW3D_SINK_AUDIOBUF_BYTES *
                channels) / (sizeof(s3d_asample_t) * 2));
            assert(framecount > 0);
            spew3d_audio_mixer_Render(
                SINKIDATA(sink)->sinksrc_mixer,
                sink->ringbuffer +
                nextnum * SPEW3D_SINK_AUDIOBUF_BYTES * channels,
                framecount
            );
            SINKIDATA(sink)->ringbufferfillnum = nextnum;
            didcopy = 1;
        } else {
            #if defined(DEBUG_SPEW3D_AUDIO_SINK_DATA)
            printf(
                "spew3d_audio_sink.c: debug: sink addr=%p: "
                "[_internal_spew3d_audio_sink_ProcessInput] "
                "sink has no playback source\n");
            #endif
            break;
        }
    }

    if (didcopy) {
        #if defined(DEBUG_SPEW3D_AUDIO_SINK_DATA)
        printf(
            "spew3d_audio_sink.c: debug: sink addr=%p: "
            "[_internal_spew3d_audio_sink_ProcessInput] "
            "playnum=%d fillnum=%d\n",
            sink,
            SINKIDATA(sink)->ringbufferplaynum,
            SINKIDATA(sink)->ringbufferfillnum);
        #endif
    } else if (!nocopyneeded) {
        #if defined(DEBUG_SPEW3D_AUDIO_SINK_DATA)
        printf(
            "spew3d_audio_sink.c: debug: sink addr=%p: "
            "[_internal_spew3d_audio_sink_ProcessInput] "
            "suspicious no copy\n");
        #endif
    }
}

S3DHID void _internal_spew3d_audio_sink_DestroyUnchecked(
        s3d_audio_sink *sink
        ) {
    assert(sink != NULL);
    assert(SINKIDATA(sink)->wasclosed);
    if (sink->type == AUDIO_SINK_OUTPUT_SDL) {
        #ifndef SPEW3D_OPTION_DISABLE_SDL
        if (SINKIDATA(sink)->output_sdlaudiodevice_opened)
            SDL_CloseAudioDevice(
                SINKIDATA(sink)->output_sdlaudiodevice);
        #endif
    }
    free(sink->soundcard_name);
    free(sink->ringbuffer);
    free(sink->internalptr);
    free(sink);
}

S3DHID void _internal_spew3d_audio_sink_MarkPermaDestroyedUnchecked(
        s3d_audio_sink *sink
        ) {
    assert(sink != NULL);
    assert(!SINKIDATA(sink)->wasclosed);
    SINKIDATA(sink)->wasclosed = 1;
}

S3DHID int _internal_spew3d_audio_sink_Process(s3d_audio_sink *sink) {
    // Handle opening the actual audio device for SDL2 output sinks:
    #ifndef SPEW3D_OPTION_DISABLE_SDL
    if (sink->type == AUDIO_SINK_OUTPUT_SDL &&
            !SINKIDATA(sink)->output_sdlaudiodevice_opened &&
            !SINKIDATA(sink)->output_sdlaudiodevice_failed) {
        char *wantedcardname = (
            sink->soundcard_name ? strdup(sink->soundcard_name) :
            NULL);
        if (sink->soundcard_name && !wantedcardname)
            goto errorfailsdlaudioopen;

        // SDL2 structure to specify the output format:
        SDL_AudioSpec wanted;
        memset(&wanted, 0, sizeof(wanted));
        wanted.freq = sink->samplerate;
        if (sizeof(s3d_asample_t) == sizeof(int32_t)) {
            wanted.format = AUDIO_S32;
        } else if (sizeof(s3d_asample_t) == sizeof(int16_t)) {
            wanted.format = AUDIO_S16;
        } else {
            fprintf(stderr, "spew3d_audio_sink.c: error: "
                "invalid unsupported sample type\n");
            goto errorfailsdlaudioopen;
        }
        wanted.channels = sink->channels;
        wanted.samples = SPEW3D_SINK_AUDIOBUF_SAMPLES_ONECHAN;
        wanted.callback = _audiocb_SDL2;
        wanted.userdata = sink;

        if (wantedcardname && strcmp(wantedcardname, "any") == 0) {
            free(wantedcardname);
            wantedcardname = NULL;
        }

        // Get the default card SDL2 recommends, if needed:
        char *sdlreporteddefaultcard = NULL;
        if (!wantedcardname) {
            SDL_AudioSpec unused = {0};
            // Get default audio info!
            #if SDL_VERSION_ATLEAST(2, 24, 0)
            if (SDL_GetDefaultAudioInfo(
                    &sdlreporteddefaultcard, &unused, 0
                    ) != 0) {
                sdlreporteddefaultcard = NULL;
            }
            #else
            sdlreporteddefaultcard = NULL;
            #endif
        }

        // Figure out which card we're actually picking:
        const char *cardname = NULL;
        int cardindex = -1;
        int c = SDL_GetNumAudioDevices(0);
        int i = 0;
        while (i < c) {
            const char *name = SDL_GetAudioDeviceName(i, 0);
            if (name && ((wantedcardname == NULL && ((
                        sdlreporteddefaultcard != NULL &&
                        strcmp(sdlreporteddefaultcard, name) == 0) ||
                        sdlreporteddefaultcard == NULL)) ||
                    (wantedcardname != NULL &&
                    s3dstrcasecmp(wantedcardname, name) == 0))) {
                cardindex = i;
                cardname = name;
                break;
            }
            i++;
        }
        if (sdlreporteddefaultcard)
            SDL_free(sdlreporteddefaultcard);  // This is from SDl2.
        free(wantedcardname);
        wantedcardname = NULL;
        if (cardindex < 0)
            goto errorfailsdlaudioopen;
        SDL_AudioDeviceID sdldev = SDL_OpenAudioDevice(
            cardname, 0, &wanted, NULL, 0
        );
        if (sdldev <= 0)
            goto errorfailsdlaudioopen;
        char *newcardname = strdup(cardname);
        if (!newcardname) {
            errorfailsdlaudioopen: ;
            free(wantedcardname);
            wantedcardname = NULL;
            SINKIDATA(sink)->output_sdlaudiodevice_failed = 1;
            #if defined(DEBUG_SPEW3D_AUDIO_SINK)
            printf(
                "spew3d_audio_sink.c: debug: sink "
                "addr=%p: opening SDL2 audio device "
                "failed: %s (SDL2 backend: \"%s\")\n",
                sink, SDL_GetError(),
                SDL_GetCurrentAudioDriver()
            );
            #endif
        } else {
            free(sink->soundcard_name);
            sink->soundcard_name = newcardname;
            SINKIDATA(sink)->output_sdlaudiodevice_opened = 1;
            SINKIDATA(sink)->output_sdlaudiodevice = sdldev;
            #if defined(DEBUG_SPEW3D_AUDIO_SINK)
            printf(
                "spew3d_audio_sink.c: debug: sink "
                "addr=%p: opened SDL2 audio device "
                "successfully (sink->soundcard_name: \"%s\", "
                "SDL2 backend: \"%s\")\n",
                sink, sink->soundcard_name,
                SDL_GetCurrentAudioDriver()
            );
            #endif
            SDL_PauseAudioDevice(
                SINKIDATA(sink)->output_sdlaudiodevice, 0
            );
        }
    }
    #endif

    // Handle closing the audio sink:
    if (SINKIDATA(sink)->wasclosed) {
        _internal_spew3d_audio_sink_DestroyUnchecked(sink);
        open_sink[open_sink_count - 1] = NULL;
        open_sink_count--;
        return 0;
    }

    // Handle feeding input audio:
    _internal_spew3d_audio_sink_ProcessInput(sink);

    return 1;
}

S3DEXP void spew3d_audio_sink_MainThreadUpdate() {
    _internal_spew3d_InitAudio();

    mutex_Lock(sink_list_mutex);
    int i = 0;
    while (i < open_sink_count) {
        if (!_internal_spew3d_audio_sink_Process(open_sink[i])) {
            // Audio sink was removed from list, don't advance
            // list index.
            continue;
        }
        i++;
    }
    #ifndef SPEW3D_OPTION_DISABLE_SDL
    uint64_t now = spew3d_time_Ticks();
    if ((sink_sdl_soundcards_refresh_ts == 0 &&
            now != 0) ||
            now + 500 > sink_sdl_soundcards_refresh_ts) {
        char **new_sdl_soundcards_list = (
            _internal_spew3d_audio_sink_GetSoundcardListOutput(
                AUDIO_SINK_OUTPUT_SDL
            ));
        if (sink_sdl_soundcards_list) {
            spew3d_stringutil_FreeArray(sink_sdl_soundcards_list);
        }
        sink_sdl_soundcards_list = new_sdl_soundcards_list;
    }
    #endif
    mutex_Release(sink_list_mutex);
}

static void _audiocb_SDL2(void *udata, uint8_t *stream, int len) {
    /*printf("_audiocb_SDL2(udata %p, stream %p, len %d)\n",
        udata, stream, len);*/
    s3d_audio_sink *sink = (s3d_audio_sink *)udata;
    if (!sink) {
        memset(stream, 0, len);
        return;
    }
    const int channels = sink->channels;
    assert(channels > 0);
    if (len >= SPEW3D_SINK_AUDIOBUF_BYTES * channels * (
            (int)sink->ringbuffersegmentcount - 1)) {
        fprintf(stderr,
            "spew3d_audio_sink.c: warning: sink "
            "addr=%p: audio misconfiguration, requested device "
            "buffer %d exceeds SPEW3D_SINK_AUDIOBUF_BYTES=%d "
            "multiplied by ring buffer segment count minus one "
            "(=%d), "
            "which means the ring buffer is inevitably too small "
            "to ever feed this request size correctly\n",
            sink, (int)len, (int)SPEW3D_SINK_AUDIOBUF_BYTES,
            (int)(int)sink->ringbuffersegmentcount - 1
        );
        fflush(stderr);
    }
    int copiedbytes = 0;
    while (copiedbytes < len) {
        assert(copiedbytes + SPEW3D_SINK_AUDIOBUF_BYTES * channels <= len);
        int prevplay = SINKIDATA(sink)->ringbufferplaynum;
        /*printf("sink output: "
            "sink %p fill %d prevplay %d "
            "segments %d\n", sink,
            (int)SINKIDATA(sink)->ringbufferfillnum,
            (int)SINKIDATA(sink)->ringbufferplaynum,
            (int)sink->ringbuffersegmentcount);*/
        if (prevplay == SINKIDATA(sink)->ringbufferfillnum) {
            if (SINKIDATA(sink)->sinksrc_type != SINKSRC_NONE) {
                #if defined(DEBUG_SPEW3D_AUDIO_SINK)
                printf(
                    "spew3d_audio_sink.c: warning: sink "
                    "addr=%p: ran out of input buffer\n",
                    sink
                );
                #endif
            }
            memset(stream, 0, len);
            return;
        }
        int nextplay = prevplay + 1;
        nextplay = (nextplay % sink->ringbuffersegmentcount);
        assert(len >= SPEW3D_SINK_AUDIOBUF_BYTES * channels);
        assert((len % SPEW3D_SINK_AUDIOBUF_BYTES * channels) == 0);
        memcpy(stream, sink->ringbuffer +
            nextplay * SPEW3D_SINK_AUDIOBUF_BYTES * channels,
            SPEW3D_SINK_AUDIOBUF_BYTES * channels);
        copiedbytes += SPEW3D_SINK_AUDIOBUF_BYTES * channels;
        SINKIDATA(sink)->ringbufferplaynum = nextplay;
        //printf("[_audiocb_SDL2] playnum=%d fillnum=%d\n",
        //    SINKIDATA(sink)->ringbufferplaynum,
        //    SINKIDATA(sink)->ringbufferfillnum);
    }
    assert(copiedbytes == len);
}

S3DEXP void spew3d_audio_sink_AddRef(s3d_audio_sink *sink) {
    SINKIDATA(sink)->refcount++;
}

S3DEXP void spew3d_audio_sink_DelRef(s3d_audio_sink *sink) {
    SINKIDATA(sink)->refcount--;
    if (SINKIDATA(sink)->refcount <= 0) {
        spew3d_audio_sink_Close(sink);
        _internal_spew3d_audio_sink_MarkPermaDestroyedUnchecked(sink);
    }
}

S3DEXP s3d_audio_sink *spew3d_audio_sink_CreateOutputEx(
        const char *soundcard_name,
        int wanttype,
        int samplerate,
        int channels,
        int buffers
        ) {
    s3d_audio_sink *asink = malloc(sizeof(*asink));
    if (!asink)
        return NULL;
    memset(asink, 0, sizeof(*asink));
    asink->internalptr = malloc(sizeof(s3d_audio_sink_internal));
    if (!asink->internalptr) {
        free(asink);
        return NULL;
    }
    memset(SINKIDATA(asink), 0, sizeof(s3d_audio_sink_internal));

    SINKIDATA(asink)->refcount++;
    asink->samplerate = samplerate;
    assert(channels > 0);
    asink->channels = channels;
    assert(buffers > 0);
    asink->ringbuffersegmentcount = buffers;
    asink->ringbuffer = malloc(
        SPEW3D_SINK_AUDIOBUF_BYTES * channels *
        asink->ringbuffersegmentcount
    );
    if (!asink->ringbuffer) {
        free(asink->internalptr);
        free(asink);
        return NULL;
    }
    memset(asink->ringbuffer, 0,
        SPEW3D_SINK_AUDIOBUF_BYTES * channels *
        asink->ringbuffersegmentcount);

    mutex_Lock(sink_list_mutex);
    s3d_audio_sink **new_open_sink = realloc(open_sink,
        sizeof(*open_sink) * (open_sink_count + 1));
    if (!new_open_sink) {
        free(asink->ringbuffer);
        free(asink->internalptr);
        free(asink);
        mutex_Release(sink_list_mutex);
        return NULL;
    }
    open_sink = new_open_sink;
    open_sink[open_sink_count] = asink;
    open_sink_count++;

    #ifndef SPEW3D_OPTION_DISABLE_SDL
    if (wanttype == AUDIO_SINK_OUTPUT_UNSPECIFIED ||
            wanttype == AUDIO_SINK_OUTPUT_SDL) {
        asink->type = AUDIO_SINK_OUTPUT_SDL;
        #if defined(DEBUG_SPEW3D_AUDIO_SINK)
        printf(
            "spew3d_audio_sink.c: debug: sink "
            "addr=%p: created as AUDIO_SINK_OUTPUT_SDL\n",
            asink
        );
        #endif

        if (soundcard_name &&
                strcmp(soundcard_name, "any") != 0) {
            asink->soundcard_name = strdup(soundcard_name);
            if (!asink->soundcard_name)
                goto failure;
        }
        mutex_Release(sink_list_mutex);
        return asink;
    }
    #endif 
    if (wanttype == AUDIO_SINK_OUTPUT_UNSPECIFIED ||
            wanttype == AUDIO_SINK_OUTPUT_VOID) {
        asink->type = AUDIO_SINK_OUTPUT_VOID;
        #if defined(DEBUG_SPEW3D_AUDIO_SINK)
        printf(
            "spew3d_audio_sink.c: debug: sink "
            "addr=%p: created as AUDIO_SINK_OUTPUT_VOID\n",
            asink
        );
        #endif
        mutex_Release(sink_list_mutex);
        return asink;
    }

    failure: ;
    spew3d_audio_sink_Close(asink);
    _internal_spew3d_audio_sink_DestroyUnchecked(asink);
    open_sink[open_sink_count - 1] = NULL;
    open_sink_count--;
    mutex_Release(sink_list_mutex);
    return NULL;
}

s3d_audio_sink *spew3d_audio_sink_CreateStereoOutput(int samplerate) {
    return spew3d_audio_sink_CreateOutputEx(
        NULL, AUDIO_SINK_OUTPUT_UNSPECIFIED, samplerate, 2, 6
    );
}

static char **
        _internal_spew3d_audio_sink_GetSoundcardListOutput(
            int type
        ) {
    #ifndef SPEW3D_OPTION_DISABLE_SDL
    if (type == AUDIO_SINK_OUTPUT_UNSPECIFIED ||
            type == AUDIO_SINK_OUTPUT_SDL) {
        int c = SDL_GetNumAudioDevices(0);
        char **result = malloc(sizeof(char*) * (c + 1));
        if (!result) {
            return NULL;
        }
        int resultcount = 0;
        int i = 0;
        while (i < c) {
            const char *name = SDL_GetAudioDeviceName(i, 0);
            if (name) {
                result[resultcount] = strdup(name);
                if (!result[resultcount]) {
                    int k = 0;
                    while (k < resultcount) {
                        free(result[resultcount]);
                        k++;
                    }
                    free(result);
                    return NULL;
                }
                resultcount++;
            }
            i++;
        }
        result[resultcount] = NULL;
        return result;
    }
    #endif
    if (type == AUDIO_SINK_OUTPUT_UNSPECIFIED ||
            type == AUDIO_SINK_OUTPUT_VOID) {
        char **result = malloc(sizeof(char*) * 2);
        if (!result) {
            return NULL;
        }
        result[0] = strdup("void soundcard");
        if (!result[0]) {
            free(result);
            return NULL;
        }
        result[1] = NULL;
        return result;
    }
    return NULL;
}

S3DEXP char **spew3d_audio_sink_GetSoundcardListOutput(
        int sinktype
        ) {
    #ifndef SPEW3D_OPTION_DISABLE_SDL
    if (sinktype == AUDIO_SINK_OUTPUT_UNSPECIFIED ||
            sinktype == AUDIO_SINK_OUTPUT_SDL) {
        mutex_Lock(sink_list_mutex);
        int i = 0;
        char **s = sink_sdl_soundcards_list;
        while (s[i]) i++;
        char **result = malloc(sizeof(char*) * (i + 1));
        if (!result) {
            mutex_Release(sink_list_mutex);
            return NULL;
        }
        memcpy(result, sink_sdl_soundcards_list,
            sizeof(char*) * (i + 1));
        mutex_Release(sink_list_mutex);
        return result;
    }
    #endif
    return _internal_spew3d_audio_sink_GetSoundcardListOutput(sinktype);
}

S3DEXP void spew3d_audio_sink_Close(s3d_audio_sink *sink) {
    assert(sink != NULL);
    assert(!SINKIDATA(sink)->wasclosed);
    SINKIDATA(sink)->wasclosed = 1;
}

#endif  // SPEW3D_IMPLEMENTATION

