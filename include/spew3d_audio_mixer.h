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
    
#ifndef SPEW3D_AUDIO_MIXER_H_
#define SPEW3D_AUDIO_MIXER_H_

typedef int64_t soundid_t;
typedef struct s3d_audio_mixer s3d_audio_mixer;
typedef struct s3d_pos s3d_pos;
typedef struct s3d_scene3d s3d_scene3d;
typedef struct s3d_obj3d s3d_obj3d;

/** Create a new audio mixer. Please note that usually,
 *  you would want to just use spew3d_audio_sink_GetMixer()
 *  on an audio sink to get a mixer instead.
 * 
 *  **Fully thread-safe.** You can create multiple mixer
 *  at once from multiple threads at any time without
 *  synchronization.
 */
S3DEXP s3d_audio_mixer *spew3d_audio_mixer_New(
    int samplerate, int speaker_channels
);

/** Play a regular sound without any 3d placement.
 * 
 *  **Fully thread-safe.** You can call this on the same
 *  mixer from multiple threads without synchronization.
 */
S3DEXP soundid_t spew3d_audio_mixer_PlayFile(
    s3d_audio_mixer *m,
    const char *sound_path,
    double volume, int looped
);

/** Play a 3d sound with placement in a 3d scene.
 *  You have to update the mixer with a scene and 3d
 *  camera for this to work, see
 *  spew3d_audio_mixer_Update3DPosition();
 * 
 *  **Fully thread-safe.** You can call this on the same
 *  mixer from multiple threads without synchronization.
 */
S3DEXP soundid_t spew3d_audio_mixer_PlayFile3DPosition(
    s3d_audio_mixer *m,
    const char *sound_path,
    double volume, s3d_pos position, double reach,
    int looped
);

S3DEXP void spew3d_audio_mixer_Update3DPosition(
    s3d_scene3d *scene, s3d_obj3d *camera
);

/** Extended version of @{spew3d_audio_mixer_PlayFile3D}.
 */
S3DEXP soundid_t spew3d_audio_mixer_PlayFile3DPositionalEx(
    s3d_audio_mixer *m,
    const char *sound_path, int vfsflags,
    double volume, s3d_pos position, double reach,
    int16_t priority, int looped
);

/** Extended version of @{spew3d_audio_mixer_PlayFile}.
 */
S3DEXP soundid_t spew3d_audio_mixer_PlayFileEx(
    s3d_audio_mixer *m,
    const char *sound_path, int vfsflags,
    double volume, double pan, int16_t priority,
    int looped
);

/** This is an advanced expert function. Render out
 *  raw audio directly, in case you haven't hooked up
 *  the mixer to a sink already (in which case the sink
 *  will call this for you instead).
 **/
S3DEXP void spew3d_audio_mixer_Render(
    s3d_audio_mixer *m,
    char *sample_buf, int frames
);

/** Play a regular sound without any 3d placement.
 * 
 *  **Partially thread-safe.** You can destroy a mixer
 *  from any thread as long as no other thread is
 *  currently using this exact same mixer.
 **/
S3DEXP void spew3d_audio_mixer_Destroy(
    s3d_audio_mixer *m
);

#endif  // SPEW3D_AUDIO_MIXER_H_

