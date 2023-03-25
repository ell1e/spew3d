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

#ifndef SPEW3D_AUDIO_SINK_H_
#define SPEW3D_AUDIO_SINK_H_

#include <stdint.h>
#ifndef SPEW3D_OPTION_DISABLE_SDL
#include <SDL2/SDL.h>
#endif

typedef enum s3d_audio_sink_type {
    AUDIO_SINK_OUTPUT_UNSPECIFIED = 0,
    AUDIO_SINK_OUTPUT_VOID = 1,
    AUDIO_SINK_OUTPUT_SDL = 2,
} s3d_audio_sink_type;

typedef struct spew3d_audio_sink {
    int refcount, wasclosed, samplerate;  // Read-only, don't touch!
    char *soundcard_name;

    s3d_audio_sink_type type;
    union {
        #ifndef SPEW3D_OPTION_DISABLE_SDL
        int output_sdlaudiodevice_opened,
            output_sdlaudiodevice_failed;
        SDL_AudioDeviceID output_sdlaudiodevice;
        #endif
    };

    int ringbuffersegmentcount;
    char *ringbuffer;
} spew3d_audio_sink;

/** This function must be called from the main thread regularly.
 *  It processes some sink creation and deletion parts that must happen
 *  on the main thread for you, so if you call it every 10ms then
 *  you can expect a 10ms delay in some operations.
 *  This function MUST NEVER be called from another thread than the
 *  main thread.
 */
void spew3d_audio_sink_MainThreadUpdate();

/** Create a new audio output sink with more extended parameters.
 *  You can call this from any thread, but you must use
 *  some sort of synchronization mechanism to call other sink
 *  functions on the returned sink from multiple threads.
 *  Remember you need to call audio_sink_MainThreadUpdate() on
 *  the main thread continuously for audio to keep working.
 */
spew3d_audio_sink *spew3d_audio_sink_CreateOutputEx(
    const char *soundcard_name, int wantsinktype,
    int samplerate, int buffers
);

/** Create a new audio output sink.
 *  You can call this from any thread, but you must use
 *  some sort of synchronization mechanism to call other sink
 *  functions on the returned sink from multiple threads.
 *  Remember you need to call audio_sink_MainThreadUpdate() on
 *  the main thread continuously for audio to keep working.
 */
spew3d_audio_sink *spew3d_audio_sink_CreateOutput(int samplerate);

/** List the available sound card names for any audio sink to use.
 *  You can call this from any thread. The last string array entry
 *  is NULL. You must free the array, for which you can use
 *  spew3d_stringutil_FreeArray().
 */
char **spew3d_audio_sink_GetSoundcardListOutput(int sinktype);

/* Internal function. */
void _internal_spew3d_audio_sink_MarkPermaDestroyedUnchecked(
    spew3d_audio_sink *sink);

/** Close the audio sink. Make sure no other threads are still using it
 *  before you do that. */
void spew3d_audio_sink_Close(spew3d_audio_sink *sink);

/** For sinks that automatically close after all other things using it
 *  disowned it.
 *  This is also used by other Spew3D components that consume sinks
 *  and offer to auto-close them for you once unused, but you can
 *  also use this yourself: Once you added a reference once via
 *  audio_sink_AddRef(), any subsequent reference deletion via
 *  audio_sink_DelRef() will reap and close the sink once the
 *  reference count is down to zero and no users are left.
 *  You can call this from any thread, but like all other functions
 *  operating on sinks after creation you must use some synchronization
 *  if you use it from multiple threads.
 */
static inline void spew3d_audio_sink_AddRef(spew3d_audio_sink *sink) {
    sink->refcount++;
}

/** For sinks that automatically close once unused/disowned.
 *  Also see audio_sink_AddRef().
 */
static inline void spew3d_audio_sink_DelRef(spew3d_audio_sink *sink) {
    sink->refcount--;
    if (sink->refcount <= 0) {
        spew3d_audio_sink_Close(sink);
        _internal_spew3d_audio_sink_MarkPermaDestroyedUnchecked(sink);
    }    
}

#endif  // SPEW3D_AUDIO_SINK_H_

