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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#ifndef SPEW3D_OPTION_DISABLE_SDL
#include <SDL2/SDL.h>
#endif

static spew3d_audio_sink **open_sink = NULL;
static volatile int32_t open_sink_count = 0;
static s3d_mutex *sink_list_mutex = NULL;
static char **sink_sdl_soundcards_list = NULL;
static int64_t sink_sdl_soundcards_refresh_ts = 0;
static char **_internal_spew3d_audio_sink_GetSoundcardListOutput(int type);
static void _audiocb_SDL2(void *udata, uint8_t *stream, int len);

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

void _internal_spew3d_audio_sink_DestroyUnchecked(
        spew3d_audio_sink *sink
        ) {
    assert(sink != NULL);
    assert(sink->wasclosed);
    if (sink->type == AUDIO_SINK_OUTPUT_SDL) {
        if (sink->output_sdlaudiodevice_opened)
            SDL_CloseAudioDevice(sink->output_sdlaudiodevice);
    }
    free(sink->soundcard_name);
    free(sink->ringbuffer);
    free(sink);
}

void _internal_spew3d_audio_sink_MarkPermaDestroyedUnchecked(
        spew3d_audio_sink *sink
        ) {
    assert(sink != NULL);
    assert(!sink->wasclosed);
    sink->wasclosed = 1;
}

int _internal_spew3d_audio_sink_Process(spew3d_audio_sink *sink) {
    // Handle opening the actual audio device for SDL2 output sinks:
    #ifndef SPEW3D_OPTION_DISABLE_SDL
    if (sink->type == AUDIO_SINK_OUTPUT_SDL &&
            !sink->output_sdlaudiodevice_opened &&
            !sink->output_sdlaudiodevice_failed) {
        SDL_AudioSpec wanted;
        memset(&wanted, 0, sizeof(wanted));
        wanted.freq = sink->samplerate;
        if (sizeof(s3d_asample_t) == sizeof(int32_t)) {
            wanted.format = AUDIO_S32;
        } else if (sizeof(s3d_asample_t) == sizeof(int16_t)) {
            wanted.format = AUDIO_S16;
        }
        wanted.channels = 2;
        wanted.samples = SPEW3D_SINK_AUDIOBUF_SAMPLES;
        wanted.callback = _audiocb_SDL2;
        wanted.userdata = sink;

        char *soundcard_name = sink->soundcard_name;
        sink->soundcard_name = NULL;
        if (soundcard_name && strcmp(soundcard_name, "any") == 0) {
            free(soundcard_name);
            soundcard_name = NULL;
        }
        const char *cardname = NULL;
        int cardindex = -1;
        int c = SDL_GetNumAudioDevices(0);
        int i = 0;
        while (i < c) {
            const char *name = SDL_GetAudioDeviceName(i, 0);
            if (name && (soundcard_name == NULL ||
                    strcasecmp(soundcard_name, name) == 0)) {
                cardindex = i;
                cardname = name;
                break;
            }
            i++;
        }
        free(soundcard_name);
        if (cardindex < 0)
            goto errorfailsdlaudioopen;
        SDL_AudioDeviceID sdldev = SDL_OpenAudioDevice(
            soundcard_name, 0, &wanted, NULL, 0
        );
        if (sdldev <= 0)
            goto errorfailsdlaudioopen;
        sink->soundcard_name = strdup(cardname);
        if (!sink->soundcard_name) {
            errorfailsdlaudioopen: ;
            sink->output_sdlaudiodevice_failed = 1;
        } else {
            sink->output_sdlaudiodevice_opened = 1;
            sink->output_sdlaudiodevice = sdldev;
        }
    }
    #endif

    // Handle closing the audio sink:
    if (sink->wasclosed) {
        _internal_spew3d_audio_sink_DestroyUnchecked(sink);
        open_sink[open_sink_count - 1] = NULL;
        open_sink_count--;
        return 0;
    }

    return 1;
}

void spew3d_audio_sink_MainThreadUpdate() {
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
    spew3d_audio_sink *sink = (spew3d_audio_sink *)udata;
    if (!sink || 1) {
        memset(stream, 0, len);
        return;
    }
}

spew3d_audio_sink *spew3d_audio_sink_CreateOutputEx(
        const char *soundcard_name,
        int wanttype,
        int samplerate,
        int buffers
        ) {
    spew3d_audio_sink *asink = malloc(sizeof(*asink));
    if (!asink)
        return NULL;
    memset(asink, 0, sizeof(*asink));
    asink->refcount++;
    asink->samplerate = samplerate;
    assert(buffers > 0);
    asink->ringbuffersegmentcount = buffers;
    asink->ringbuffer = malloc(
        SPEW3D_SINK_AUDIOBUF_BYTES *
        asink->ringbuffersegmentcount
    );
    if (!asink->ringbuffer) {
        free(asink);
        return NULL;
    }
    memset(asink->ringbuffer, 0,
        SPEW3D_SINK_AUDIOBUF_BYTES *
        asink->ringbuffersegmentcount);

    mutex_Lock(sink_list_mutex);
    spew3d_audio_sink **new_open_sink = realloc(open_sink,
        sizeof(*open_sink) * (open_sink_count + 1));
    if (!new_open_sink) {
        free(asink->ringbuffer);
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
        if (soundcard_name &&
                strcmp(soundcard_name, "any") != 0) {
            asink->soundcard_name = strdup(soundcard_name);
            if (!asink->soundcard_name)
                goto failure;
        }
        return asink;
    }
    #endif 
    if (wanttype == AUDIO_SINK_OUTPUT_UNSPECIFIED ||
            wanttype == AUDIO_SINK_OUTPUT_VOID) {
        asink->type = AUDIO_SINK_OUTPUT_VOID;
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

spew3d_audio_sink *spew3d_audio_sink_CreateOutput(int samplerate) {
    return spew3d_audio_sink_CreateOutputEx(
        NULL, AUDIO_SINK_OUTPUT_UNSPECIFIED, samplerate, 4
    );
}

char **_internal_spew3d_audio_sink_GetSoundcardListOutput(int type) {
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

char **spew3d_audio_sink_GetSoundcardListOutput(int sinktype) {
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

void spew3d_audio_sink_Close(spew3d_audio_sink *sink) {
    assert(sink != NULL);
    assert(!sink->wasclosed);
    sink->wasclosed = 1;
}

#endif  // SPEW3D_IMPLEMENTATION

