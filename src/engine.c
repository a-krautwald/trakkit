#define _POSIX_C_SOURCE 200809L

#include "engine.h"

#include <alsa/asoundlib.h>
#include <math.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#define ENGINE_SAMPLE_RATE 48000
#define ENGINE_BUFFER_FRAMES 256

typedef struct Voice {
    int channel;
    const Sample *sample;
    float pos;
    float step;
    float gain;
    StepEffectType effect;
    float effect_amount;
    float lp_state;
    float hp_state;
    int active;
} Voice;

struct EngineState {
    TrackerProject *project;
    pthread_t audio_thread;
    pthread_mutex_t lock;
    int running;
    int playing;
    snd_pcm_t *pcm;
    Voice voices[TRACKER_MAX_VOICES];
    float reverb_buf[ENGINE_SAMPLE_RATE / 2];
    int reverb_idx;
    float step_phase;
};

static float apply_effect(Voice *v, float x, EngineState *e) {
    switch (v->effect) {
        case STEP_EFFECT_DISTORTION: {
            float drive = 1.0f + v->effect_amount * 8.0f;
            return tanhf(x * drive);
        }
        case STEP_EFFECT_LOWPASS: {
            float a = 0.02f + (1.0f - v->effect_amount) * 0.25f;
            v->lp_state += a * (x - v->lp_state);
            return v->lp_state;
        }
        case STEP_EFFECT_HIGHPASS: {
            float a = 0.02f + (1.0f - v->effect_amount) * 0.25f;
            float low = v->hp_state + a * (x - v->hp_state);
            float y = x - low;
            v->hp_state = low;
            return y;
        }
        case STEP_EFFECT_REVERB: {
            int delay = ENGINE_SAMPLE_RATE / 8;
            int read_idx = (e->reverb_idx - delay + (int)(sizeof(e->reverb_buf) / sizeof(e->reverb_buf[0]))) %
                           (int)(sizeof(e->reverb_buf) / sizeof(e->reverb_buf[0]));
            float wet = e->reverb_buf[read_idx];
            float out = x * (1.0f - v->effect_amount) + wet * v->effect_amount;
            e->reverb_buf[e->reverb_idx] = x + wet * 0.4f;
            return out;
        }
        case STEP_EFFECT_NONE:
        default:
            return x;
    }
}

static void trigger_step(EngineState *e, int step) {
    for (int ch = 0; ch < TRACKER_CHANNELS; ++ch) {
        StepEvent *ev = &e->project->pattern.steps[ch][step];
        if (!ev->active) {
            continue;
        }
        ChannelState *cs = &e->project->channels[ch];
        if (cs->mute || cs->sample_count <= 0) {
            continue;
        }
        int si = ev->sample_index;
        if (si < 0 || si >= cs->sample_count) {
            si = cs->selected_sample;
        }
        if (si < 0 || si >= cs->sample_count) {
            continue;
        }
        for (int i = 0; i < TRACKER_MAX_VOICES; ++i) {
            if (!e->voices[i].active) {
                Voice *v = &e->voices[i];
                memset(v, 0, sizeof(*v));
                v->active = 1;
                v->channel = ch;
                v->sample = &cs->samples[si];
                v->pos = 0.0f;
                v->step = powf(2.0f, ev->note / 12.0f) * ((float)v->sample->sample_rate / (float)ENGINE_SAMPLE_RATE);
                v->gain = ev->volume * cs->level;
                v->effect = ev->effect;
                v->effect_amount = ev->effect_amount;
                break;
            }
        }
    }
}

static void mix_audio(EngineState *e, float *out, int frames) {
    memset(out, 0, sizeof(float) * frames * 2);

    float frames_per_step = (60.0f / (float)e->project->bpm) * ((float)ENGINE_SAMPLE_RATE / 4.0f);
    for (int f = 0; f < frames; ++f) {
        if (e->playing) {
            if (e->step_phase <= 0.0f) {
                trigger_step(e, e->project->playing_step);
                e->project->playing_step = (e->project->playing_step + 1) % TRACKER_STEPS;
                e->step_phase += frames_per_step;
            }
            e->step_phase -= 1.0f;
        }

        float l = 0.0f;
        float r = 0.0f;
        for (int i = 0; i < TRACKER_MAX_VOICES; ++i) {
            Voice *v = &e->voices[i];
            if (!v->active || !v->sample || !v->sample->data) {
                continue;
            }
            size_t idx = (size_t)v->pos;
            if (idx >= v->sample->frames) {
                v->active = 0;
                continue;
            }
            float s = 0.0f;
            if (v->sample->channels == 1) {
                s = v->sample->data[idx];
            } else {
                s = 0.5f * (v->sample->data[idx * 2] + v->sample->data[idx * 2 + 1]);
            }
            s *= v->gain;
            s = apply_effect(v, s, e);
            l += s;
            r += s;
            v->pos += v->step;
        }
        e->reverb_idx = (e->reverb_idx + 1) % (int)(sizeof(e->reverb_buf) / sizeof(e->reverb_buf[0]));
        out[f * 2] = fmaxf(-1.0f, fminf(1.0f, l));
        out[f * 2 + 1] = fmaxf(-1.0f, fminf(1.0f, r));
    }
}

static void *audio_worker(void *arg) {
    EngineState *e = (EngineState *)arg;
    int16_t pcm_out[ENGINE_BUFFER_FRAMES * 2];
    float mix_out[ENGINE_BUFFER_FRAMES * 2];

    while (e->running) {
        pthread_mutex_lock(&e->lock);
        mix_audio(e, mix_out, ENGINE_BUFFER_FRAMES);
        pthread_mutex_unlock(&e->lock);

        for (int i = 0; i < ENGINE_BUFFER_FRAMES * 2; ++i) {
            float x = mix_out[i] * 32767.0f;
            if (x > 32767.0f) x = 32767.0f;
            if (x < -32768.0f) x = -32768.0f;
            pcm_out[i] = (int16_t)x;
        }
        snd_pcm_sframes_t written = snd_pcm_writei(e->pcm, pcm_out, ENGINE_BUFFER_FRAMES);
        if (written < 0) {
            snd_pcm_prepare(e->pcm);
        }
    }
    return NULL;
}

EngineState *engine_create(TrackerProject *project) {
    if (!project) {
        return NULL;
    }
    EngineState *e = (EngineState *)calloc(1, sizeof(EngineState));
    if (!e) {
        return NULL;
    }
    e->project = project;
    pthread_mutex_init(&e->lock, NULL);
    return e;
}

void engine_destroy(EngineState *engine) {
    if (!engine) {
        return;
    }
    engine_stop(engine);
    pthread_mutex_destroy(&engine->lock);
    free(engine);
}

int engine_start(EngineState *engine) {
    if (!engine) {
        return -1;
    }

    if (snd_pcm_open(&engine->pcm, "default", SND_PCM_STREAM_PLAYBACK, 0) < 0) {
        return -1;
    }
    if (snd_pcm_set_params(engine->pcm,
                           SND_PCM_FORMAT_S16_LE,
                           SND_PCM_ACCESS_RW_INTERLEAVED,
                           2,
                           ENGINE_SAMPLE_RATE,
                           1,
                           50000) < 0) {
        snd_pcm_close(engine->pcm);
        engine->pcm = NULL;
        return -1;
    }

    engine->running = 1;
    if (pthread_create(&engine->audio_thread, NULL, audio_worker, engine) != 0) {
        engine->running = 0;
        snd_pcm_close(engine->pcm);
        engine->pcm = NULL;
        return -1;
    }
    return 0;
}

void engine_stop(EngineState *engine) {
    if (!engine || !engine->running) {
        return;
    }
    engine->running = 0;
    pthread_join(engine->audio_thread, NULL);
    if (engine->pcm) {
        snd_pcm_drain(engine->pcm);
        snd_pcm_close(engine->pcm);
        engine->pcm = NULL;
    }
}

void engine_set_playing(EngineState *engine, bool playing) {
    if (!engine) {
        return;
    }
    pthread_mutex_lock(&engine->lock);
    engine->playing = playing ? 1 : 0;
    if (!playing) {
        engine->project->playing_step = 0;
        engine->step_phase = 0.0f;
    }
    pthread_mutex_unlock(&engine->lock);
}

bool engine_is_playing(const EngineState *engine) {
    return engine && engine->playing;
}

void engine_lock(EngineState *engine) {
    if (!engine) {
        return;
    }
    pthread_mutex_lock(&engine->lock);
}

void engine_unlock(EngineState *engine) {
    if (!engine) {
        return;
    }
    pthread_mutex_unlock(&engine->lock);
}
