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

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define CHANNEL_SRC_NONE 0
#define CHANNEL_SRC_SINK 1
#define CHANNEL_SRC_DECODER 2

typedef struct s3d_audio_channel {
    int src_type;
    union {
        s3d_audio_sink *src_sink;
        s3d_audio_decoder *src_decoder;
    };
    soundid_t soundid;
    int16_t priority;
    int samplerate;
    double volume, pan;
    s3d_audio_resampler *resampler;
    int64_t start_ts;
    int stopped, loop;
    
    int is3d;
    s3d_pos pos; double reach;
    double _effectivevol, _effectivepan;
    int64_t last3dupdate_ts;
} s3d_audio_channel;

typedef struct s3d_audio_mixer {
    int samplerate, speaker_channels;
    soundid_t last_sound_id;
    s3d_mutex *m;
    s3d_audio_channel *channels;
    int channels_count;

    s3d_pos lastcampos;
    s3d_rotation lastcamangle;
    
    double anticlipfactor, anticlipthreshold;
    int64_t last_anticlip_ts;

    char *temp_mix_buf;
    int temp_mix_buf_alloc;

    int64_t *effective_vol_ints_buf;
} s3d_audio_mixer;

s3d_audio_mixer **_global_mixer_list = NULL;
int _global_mixer_list_fill = 0;
int _global_mixer_list_alloc = 0;
s3d_mutex *_global_mixer_list_mutex = NULL;

S3DHID __attribute__((constructor)) void _ensure_global_mixer_mutex() {
    if (_global_mixer_list_mutex != NULL)
        return;
    _global_mixer_list_mutex = mutex_Create();
    if (_global_mixer_list_mutex == NULL) {
        fprintf(stderr,
            "spew3d_audio_mixer.c: error: FATAL ERROR, "
            "couldn't allocate mutex.\n");
        _exit(1);
    }
}

S3DHID void spew3d_audio_mixer_UpdateAllOnMainThread() {
    uint64_t now = spew3d_time_Ticks();
    mutex_Lock(_global_mixer_list_mutex);
    int i = 0;
    while (i < _global_mixer_list_fill) {
        s3d_audio_mixer *m = _global_mixer_list[i];
        mutex_Lock(m->m);
        if (now > m->last_anticlip_ts + 5000)
            m->last_anticlip_ts = now - 100;
        while (m->last_anticlip_ts < now) {
            if (m->anticlipfactor < 0.99) {
                double inversefac = 1.0 - m->anticlipfactor;
                inversefac = inversefac * inversefac * 0.1 + 0.9 * inversefac;
                m->anticlipfactor = 1.0 - inversefac;
            } else {
                m->anticlipfactor = 1.0;
            }
            m->last_anticlip_ts += 50;
        }
        mutex_Release(m->m);
        i++;
    }
    mutex_Release(_global_mixer_list_mutex);
}

S3DEXP s3d_audio_mixer *spew3d_audio_mixer_New(
        int samplerate, int speaker_channels
        ) {
    _ensure_global_mixer_mutex();
    mutex_Lock(_global_mixer_list_mutex);
    if (_global_mixer_list_fill + 1 >
            _global_mixer_list_alloc) {
        int newalloc = _global_mixer_list_fill + 1 + 32;
        s3d_audio_mixer **newlist = realloc(
            _global_mixer_list, sizeof(*_global_mixer_list) *
            newalloc);
        if (!newlist) {
            mutex_Release(_global_mixer_list_mutex);
            return NULL;
        }
        _global_mixer_list = newlist;
        _global_mixer_list_alloc = newalloc;
    }

    s3d_audio_mixer *m = malloc(sizeof(*m));
    if (!m) {
        mutex_Release(_global_mixer_list_mutex);
        return NULL;
    }
    memset(m, 0, sizeof(*m));
    m->m = mutex_Create();
    if (!m->m) {
        free(m);
        mutex_Release(_global_mixer_list_mutex);
        return NULL;
    }
    m->channels_count = 32;
    m->channels = malloc(sizeof(*m->channels) * m->channels_count);
    if (!m->channels) {
        mutex_Destroy(m->m);
        free(m);
        mutex_Release(_global_mixer_list_mutex);
        return NULL;
    }
    memset(m->channels, 0,
        sizeof(*m->channels) * m->channels_count);
    m->samplerate = samplerate;
    m->anticlipfactor = 1.0;
    m->anticlipthreshold = 0.99;
    m->speaker_channels = speaker_channels;
    m->effective_vol_ints_buf = malloc(
        sizeof(*m->effective_vol_ints_buf) * (1+speaker_channels)
    );
    if (!m->effective_vol_ints_buf) {
        free(m->channels);
        mutex_Destroy(m->m);
        free(m);
        mutex_Release(_global_mixer_list_mutex);
        return NULL;
    }
    #if defined(DEBUG_SPEW3D_AUDIO_MIXER)
    printf(
        "spew3d_audio_mixer.c: debug: mixer "
        "addr=%p: spew3d_audio_mixer_New() "
        "created a new mixer.\n",
        m
    );
    #endif
    _global_mixer_list[_global_mixer_list_fill] = m;
    _global_mixer_list_fill++;
    mutex_Release(_global_mixer_list_mutex);
    return m;
}

S3DHID int _spew3d_audio_mixer_TryCloseChannel_nolock(
        s3d_audio_mixer *m, int channelno, int force
        ) {
    if (!force && !m->channels[channelno].stopped &&
            m->channels[channelno].src_type != CHANNEL_SRC_NONE
            ) {
        return 0;
    }
    if (m->channels[channelno].resampler != NULL) {
        s3d_audioresampler_Free(m->channels[channelno].resampler);
        m->channels[channelno].resampler = NULL;
    }
    if (m->channels[channelno].src_type == CHANNEL_SRC_SINK) {
        if (m->channels[channelno].src_sink != NULL) {
            spew3d_audio_sink_DelRef(
                m->channels[channelno].src_sink);
            m->channels[channelno].src_sink = NULL;
        }
        m->channels[channelno].src_type = CHANNEL_SRC_NONE;
    } else if (m->channels[channelno].src_type == CHANNEL_SRC_DECODER) {
        if (m->channels[channelno].src_decoder != NULL) {
            s3d_audiodecoder_Destroy(
                m->channels[channelno].src_decoder);
            m->channels[channelno].src_decoder = NULL;
        }
        m->channels[channelno].src_type = CHANNEL_SRC_NONE;
    }
    return 1;
}

S3DHID int _allocate_new_sound_on_channel_nolock(
        s3d_audio_mixer *m, int priority) {
    int lowest_prio_seen = -1;
    int lowest_prio_no = -1;
    int i = 0;
    while (i < m->channels_count) {
        if (_spew3d_audio_mixer_TryCloseChannel_nolock(
                m, i, 0)) {
            break;
        }
        if (m->channels[i].priority < lowest_prio_seen ||
                lowest_prio_seen < 0) {
            lowest_prio_seen = m->channels[i].priority;
            lowest_prio_no = i;
        }
        i++;
    }
    if (i >= m->channels_count) {
        if (lowest_prio_seen >= 0 &&
                lowest_prio_seen < priority) {
            int outcome = _spew3d_audio_mixer_TryCloseChannel_nolock(
                m, lowest_prio_no, 1
            );
            assert(outcome != 0);
            #if defined(DEBUG_SPEW3D_AUDIO_MIXER)
            printf(
                "spew3d_audio_mixer.c: debug: mixer "
                "addr=%p: _allocate_new_sound_on_channel_nolock() "
                "allocated slot for new sound with channelno=%d ("
                "overriding previous lower priority sound).\n",
                m, i
            );
            #endif
            return lowest_prio_no;
        }
        return -1;
    }
    #if defined(DEBUG_SPEW3D_AUDIO_MIXER)
    printf(
        "spew3d_audio_mixer.c: debug: mixer "
        "addr=%p: _allocate_new_sound_on_channel_nolock() "
        "allocated slot for new sound with channelno=%d ("
        "empty unused channel).\n",
        m, i
    );
    #endif
    return i;
}

S3DHID void _spew3d_audio_mixer_Do3DUpdate_nolock(
        s3d_audio_mixer *m, int channelno, int64_t ts
       ) {
    s3d_audio_channel *c = &m->channels[channelno];
    if (!c->is3d || ts < c->last3dupdate_ts + 50 ||
            c->stopped || c->src_type == CHANNEL_SRC_NONE
            )
        return;
    double dist = spew3d_math3d_dist(
        &c->pos, &m->lastcampos
    );
    double linear_fac = fmax(0.0, fmin(1.0, dist / fmax(0.001,
        c->reach
    )));
    double exp_fac = linear_fac * linear_fac;
    double mixed_fac = 0.7 * linear_fac + 0.3 * exp_fac;
    double effectivevol = fmin(2.0, fmax(0.0,
        fmax(0.0, fmin(1.0, mixed_fac)) *
        c->volume));
    c->_effectivevol = effectivevol;
    c->_effectivepan = 0;  // FIXME.
    c->last3dupdate_ts = ts;
}

S3DEXP void spew3d_audio_mixer_Render(
        s3d_audio_mixer *m,
        char *sample_buf, int frames
        ) {
    uint64_t ts = spew3d_time_Ticks();
    mutex_Lock(m->m);
    int renderbytes = m->speaker_channels * frames *
        sizeof(s3d_asample_t);
    if (m->temp_mix_buf_alloc < renderbytes) {
        int newalloc = renderbytes;
        if (newalloc < 1024 * 10)
            newalloc = 1024 * 10;
        m->temp_mix_buf = malloc(newalloc);
        if (!m->temp_mix_buf) {
            memset(sample_buf, 0, renderbytes);
            mutex_Release(m->m);
            return;
        }
        m->temp_mix_buf_alloc = renderbytes;
    }

    memset(sample_buf, 0,
        frames * m->speaker_channels * sizeof(s3d_asample_t));

    char *tmpbuf = m->temp_mix_buf;
    memset(sample_buf, 0, renderbytes);
    int i = 0;
    while (i < m->channels_count) {
        _spew3d_audio_mixer_Do3DUpdate_nolock(m, i, ts);
        s3d_audio_channel *c = &m->channels[i];
        if (c->src_type == CHANNEL_SRC_NONE || c->stopped
                ) {
            i++;
            continue;
        }
        memset(tmpbuf, 0, renderbytes);
        int renderedframes = 0;
        if (c->src_type == CHANNEL_SRC_DECODER) {
            int resetattempts = 0;
            char *writeat = tmpbuf;
            while (renderedframes < frames) {
                int wantframes = frames - renderedframes;
                s3d_audio_decoder *d = c->src_decoder;
                int haderror = 0;
                int newframes = s3d_audiodecoder_Decode(
                    d, writeat, wantframes,
                    &haderror
                );
                if (haderror || s3d_audiodecoder_HadError(d)) {
                    int outcome = _spew3d_audio_mixer_TryCloseChannel_nolock(
                        m, i, 1
                    );
                    c->stopped = 1;
                    assert(outcome != 0);
                    break;
                }
                if (newframes == 0) {
                    if (c->loop && resetattempts < 10) {
                        resetattempts++;
                        s3d_audiodecoder_ResetToStart(d);
                        continue;
                    }
                    c->stopped = 1;
                    break;
                }
                writeat += (newframes * m->speaker_channels *
                    sizeof(s3d_asample_t));
                renderedframes += newframes;
            }
        }

        //  Now adjust what we rendered for volume & panning
        int outputchannels = m->speaker_channels;
        double evolume = c->_effectivevol;
        double epan = c->_effectivepan;
        int64_t max = INT16_MAX;
        int64_t min = -max;
        if (sizeof(s3d_asample_t) == 4) {
            max = INT32_MAX;
            min = -max;
        }
        int64_t evolume_left_int = (
            max * evolume * (1.0 - fmax(0.0, fmin(1.0, epan)))
        );
        if (evolume_left_int > max * 2) evolume_left_int = max * 2;
        if (evolume_left_int < 0) evolume_left_int = 0;
        int64_t evolume_right_int = (
            max * evolume * (1.0 + (fmax(-1.0, fmin(0.0, epan))))
        );
        if (evolume_right_int > max * 2) evolume_right_int = max * 2;
        if (evolume_right_int < 0) evolume_right_int = 0;
        int ispanned = fabs(epan) > 0.01;
        int64_t evolume_int = evolume_left_int;
        int mightclip = (evolume_left_int > max ||
            evolume_right_int > max);

        char *writeat2 = tmpbuf;
        if (!ispanned && !mightclip) {
            // For performance, do this in a separate loop with
            // less conditionals per sample:
            int i2 = 0;
            while (i2 < frames) {
                int i3 = 0;
                while (i3 < outputchannels) {
                    int64_t value = *((s3d_asample_t*)(
                        writeat2
                    ));
                    value *= evolume_int;
                    value /= max;
                    *((s3d_asample_t*)writeat2) = value;
                    writeat2 += sizeof(s3d_asample_t);
                    i3++;
                }
                i2++;
            }
        } else if (!ispanned) {
            int i2 = 0;
            while (i2 < frames) {
                int i3 = 0;
                while (i3 < outputchannels) {
                    int64_t value = *((s3d_asample_t*)(
                        writeat2
                    ));
                    value *= evolume_int;
                    value /= max;
                    if (value > max) value = max;
                    if (value < min) value = min;
                    *((s3d_asample_t*)writeat2) = value;
                    writeat2 += sizeof(s3d_asample_t);
                    i3++;
                }
                i2++;
            }
        } else {
            int64_t *vol_ints_buf = m->effective_vol_ints_buf;
            int i2 = 0;
            while (i2 < outputchannels) {
                if (i2 == 0) {
                    vol_ints_buf[i2] = evolume_left_int;
                } else if (i2 == 1) {
                    vol_ints_buf[i2] = evolume_right_int;
                } else {
                    // FIXME: do proper multi-channel audio here.
                    vol_ints_buf[i2] = evolume_int;
                }
                i2++;
            }
            i2 = 0;
            while (i2 < frames) {
                int64_t *read_vol_ptr = vol_ints_buf;
                int i3 = 0;
                while (i3 < m->speaker_channels) {
                    int64_t value = *((s3d_asample_t*)(
                        writeat2
                    ));
                    value *= max;
                    value /= *read_vol_ptr;
                    if (value > max) value = max;
                    if (value < min) value = min;
                    *((s3d_asample_t*)writeat2) = value;
                    writeat2 += sizeof(s3d_asample_t);
                    read_vol_ptr++;
                    i3++;
                }
                i2++;
            }
        }
        
        // Finally, mix it into the result:
        s3d_asample_t *mix_to = (s3d_asample_t*)sample_buf;
        s3d_asample_t *mix_from = (s3d_asample_t*)tmpbuf;
        s3d_asample_t *mix_end = (s3d_asample_t*)(
            (char *)tmpbuf + frames * outputchannels *
            sizeof(s3d_asample_t)
        );
        int64_t anticlip_int_fac = (double)max * (double)m->anticlipfactor;
        int64_t triggerthreshold = (double)max * (double)m->anticlipthreshold;
        double triggerthresholdf = triggerthreshold;
        while (mix_from != mix_end) {
            int64_t value = *((s3d_asample_t *)mix_to);
            int64_t value2 = ((int64_t)*((s3d_asample_t *)mix_from) *
                (int64_t)8) / (int64_t)10;
            value2 = value2 * (int64_t)anticlip_int_fac / (int64_t)max;
            int64_t newvalue = value + value2;
            int64_t newvalueabs = (newvalue > 0 ? newvalue : -newvalue);
            if (newvalueabs > triggerthreshold) {
                double new_anti_clip = (triggerthresholdf / (double)newvalueabs);
                new_anti_clip = fmax(0.6, fmin(1.0, new_anti_clip));
                if (new_anti_clip < m->anticlipfactor)
                    m->anticlipfactor = new_anti_clip;
                anticlip_int_fac = (double)max * (double)m->anticlipfactor;
            }
            if (newvalue > max) newvalue = max;
            if (newvalue < min) newvalue = min;
            *((s3d_asample_t *)mix_to) = (s3d_asample_t)newvalue;
            mix_from++;
            mix_to++;
        }
        i++;
    }
    mutex_Release(m->m);
}

S3DHID soundid_t _spew3d_audio_mixer_PlayFileExEx(
        s3d_audio_mixer *m,
        const char *sound_path, int vfsflags,
        double volume, double pan, double reach,
        int is3d, s3d_pos pos, int16_t priority, int looped) {
    if (priority < 0) priority = 0;
    int64_t now = spew3d_time_Ticks();
    mutex_Lock(m->m);
    int channelno = _allocate_new_sound_on_channel_nolock(
        m, priority
    );
    if (channelno < 0) {
        mutex_Release(m->m);
        return -1;
    }
    m->last_sound_id++;
    int64_t newid = m->last_sound_id;
    memset(&m->channels[channelno], 0,
        sizeof(m->channels[channelno]));
    m->channels[channelno].soundid = newid;
    m->channels[channelno].src_type = CHANNEL_SRC_DECODER;
    mutex_Release(m->m);
    s3d_audio_decoder *d = audiodecoder_NewFromFileEx(
        sound_path, vfsflags
    );
    if (!d) {
        mutex_Lock(m->m);
        if (m->channels[channelno].soundid != newid) {
            // Somebody else took that channel over.
            mutex_Release(m->m);
            return -1;
        }
        m->channels[channelno].src_type = CHANNEL_SRC_NONE;
        m->channels[channelno].stopped = 1;
        mutex_Release(m->m);
        return -1;
    }
    s3d_audiodecoder_SetResampleTo(d, m->samplerate);
    mutex_Lock(m->m);
    if (m->channels[channelno].soundid != newid) {
        // Somebody else took that channel over.
        s3d_audiodecoder_Destroy(d);
        mutex_Release(m->m);
        return -1;
    }
    m->channels[channelno].src_decoder = d;
    if (is3d) {
        m->channels[channelno].is3d = 1;
        m->channels[channelno].pos = pos;
        m->channels[channelno].pan = pan;
        m->channels[channelno].reach = reach;
    }
    m->channels[channelno].volume = fmax(0, volume);
    if (!is3d) {
        m->channels[channelno]._effectivevol =
            fmin(2.0, fmax(0.0, volume));
        m->channels[channelno]._effectivepan =
            fmin(1.0, fmax(-1.0, pan));
    }
    m->channels[channelno].loop = looped;
    mutex_Release(m->m);
    return -1;
}

S3DEXP soundid_t spew3d_audio_mixer_PlayFileEx(
        s3d_audio_mixer *m,
        const char *sound_path, int vfsflags,
        double volume, double pan, int16_t priority,
        int looped) {
    assert(m != NULL);
    s3d_pos emptypos = {0};
    return _spew3d_audio_mixer_PlayFileExEx(
        m, sound_path, vfsflags, volume, pan, 0,
        0, emptypos, priority, looped
    );
}

S3DEXP soundid_t spew3d_audio_mixer_PlayFile(
        s3d_audio_mixer *m,
        const char *sound_path,
        double volume, int looped
        ) {
    assert(m != NULL);
    return spew3d_audio_mixer_PlayFileEx(
        m, sound_path, 0, volume, 0, 0,
        looped
    );
}

S3DEXP soundid_t spew3d_audio_mixer_PlayFile3DEx(
        s3d_audio_mixer *m,
        const char *sound_path, int vfsflags,
        double volume, s3d_pos position, double reach,
        int16_t priority, int looped
        ) {
    assert(m != NULL);
    return _spew3d_audio_mixer_PlayFileExEx(
        m, sound_path, vfsflags, volume, 0, reach,
        1, position, priority, looped
    );
}

S3DEXP soundid_t spew3d_audio_mixer_PlayFile3D(
        s3d_audio_mixer *m,
        const char *sound_path,
        double volume, s3d_pos position, double reach,
        int looped
        ) {
    return spew3d_audio_mixer_PlayFile3DEx(
        m, sound_path, 0, volume, position, reach,
        0, looped
    );
}

S3DEXP void spew3d_audio_mixer_Destroy(
        s3d_audio_mixer *m
        ) {
    if (!m)
        return;
    int i = 0;
    while (i < m->channels_count) {
        int result = _spew3d_audio_mixer_TryCloseChannel_nolock(
            m, i, 1
        );
        assert(result != 0);
        i++;
    }
    free(m->channels);
    mutex_Destroy(m->m);
    free(m);
}

#endif  // SPEW3D_IMPLEMENTATION

