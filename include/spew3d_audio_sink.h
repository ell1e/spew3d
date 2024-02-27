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

#ifndef SPEW3D_AUDIO_SINK_H_
#define SPEW3D_AUDIO_SINK_H_

#include <stdint.h>
#ifndef SPEW3D_OPTION_DISABLE_SDL
#include <SDL2/SDL.h>
#endif

typedef struct s3d_audio_mixer s3d_audio_mixer;

typedef enum s3d_audio_sink_type {
    AUDIO_SINK_OUTPUT_UNSPECIFIED = 0,
    AUDIO_SINK_OUTPUT_VOID = 1,
    AUDIO_SINK_OUTPUT_SDL = 2,
} s3d_audio_sink_type;

typedef struct s3d_audio_sink {
    int samplerate;
    int channels;
    char *soundcard_name;

    s3d_audio_sink_type type;
    int ringbuffersegmentcount;
    char *ringbuffer;

    void *internalptr;  // Read-only, don't touch!
} s3d_audio_sink;

/** This function must be called from the main thread regularly.
 *  It processes some sink creation and deletion parts that must happen
 *  on the main thread for you, so if you call it every 10ms then
 *  you can expect a 10ms delay in some operations.
 *  This function MUST NEVER be called from another thread than the
 *  main thread.
 */
S3DEXP void spew3d_audio_sink_MainThreadUpdate();

/** Create a new audio output sink with more extended parameters.
 *  You can call this from any thread, but you must use
 *  some sort of synchronization mechanism to call other sink
 *  functions on the returned sink from multiple threads.
 *  Remember you need to call audio_sink_MainThreadUpdate() on
 *  the main thread continuously for audio to keep working.
 *
 *  **Partially thread-safe.**
 *  While creating a sink is thread-safe, otherwise it can only
 *  ever safely be operated and eventually closed down by a
 *  single thread. The exception is if you attach a mixer,
 *  which is thread-safe.
 */
S3DEXP s3d_audio_sink *spew3d_audio_sink_CreateOutputEx(
    const char *soundcard_name, int wantsinktype,
    int samplerate, int channels, int buffers
);

/** Create a new audio output sink.
 *  You can call this from any thread, but you must use
 *  some sort of synchronization mechanism to call other sink
 *  functions on the returned sink from multiple threads.
 *  Remember you need to call audio_sink_MainThreadUpdate() on
 *  the main thread continuously for audio to keep working.
 *
 *  **Partially thread-safe.**
 *  See @{spew3d_audio_sink_CreateOutputEx} for details.
 */
S3DEXP s3d_audio_sink *
    spew3d_audio_sink_CreateStereoOutput(int samplerate);

/** List the available sound card names for any audio sink to use.
 *  You can call this from any thread. The last string array entry
 *  is NULL. You must free the array, for which you can use
 *  spew3d_stringutil_FreeArray().
 */
S3DEXP char **s3d_audio_sink_GetSoundcardListOutput(int sinktype);

/** Close the audio sink. Make sure no other threads are still using it
 *  before you do that. */
S3DEXP void spew3d_audio_sink_Close(s3d_audio_sink *sink);

/** Set a sink to play audio coming from a decoder. (This is
 * ADVANCED EXPERT functionality.)
 * The audio will be resampled and channel adjusted automatically
 * for the given sink. Please note an audio decoder can only feed
 * into one sink at a time or things will break badly.
 */
S3DEXP int spew3d_audio_sink_FeedFromDecoder(
    s3d_audio_sink *sink, s3d_audio_decoder *decoder
);

/** Set the sink to use a mixer if it doesn't yet, and return
 *  the audio mixer. This is what you would usually want to do
 *  with a sink. A mixer can be used to simulate a complex, rich
 *  audio world with multiple sound sources.
 *
 *  **Fully thread-safe.**
 *  This function is thread-safe, as in you can call it from
 *  multiple differen threads at any time with no synchronization.
 *  Mixers themselves are also thread-safe to use.
 **/
S3DEXP s3d_audio_mixer *spew3d_audio_sink_GetMixer(
    s3d_audio_sink *sink
);

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
S3DEXP void spew3d_audio_sink_AddRef(s3d_audio_sink *sink);

/** For sinks that automatically close once unused/disowned.
 *  Also see audio_sink_AddRef().
 */
S3DEXP void spew3d_audio_sink_DelRef(s3d_audio_sink *sink);

/// This is usually called by sdl_event.c for you, as part of
/// spew3d_event_UpdateMainThread().
S3DEXP void spew3d_audio_sink_InternalMainThreadUpdate();

#endif  // SPEW3D_AUDIO_SINK_H_

