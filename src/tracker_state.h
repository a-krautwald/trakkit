#ifndef TRACKER_STATE_H
#define TRACKER_STATE_H

#include <stdbool.h>
#include <stddef.h>

#define TRACKER_CHANNELS 8
#define TRACKER_STEPS 64
#define TRACKER_MAX_SAMPLES_PER_CHANNEL 128
#define TRACKER_MAX_SAMPLE_NAME 128
#define TRACKER_MAX_SAMPLE_PATH 512
#define TRACKER_MAX_VOICES 64

typedef enum StepEffectType {
    STEP_EFFECT_NONE = 0,
    STEP_EFFECT_DISTORTION = 1,
    STEP_EFFECT_LOWPASS = 2,
    STEP_EFFECT_HIGHPASS = 3,
    STEP_EFFECT_REVERB = 4
} StepEffectType;

typedef struct StepEvent {
    bool active;
    int sample_index;
    int note;
    float volume;
    StepEffectType effect;
    float effect_amount;
} StepEvent;

typedef struct Pattern {
    StepEvent steps[TRACKER_CHANNELS][TRACKER_STEPS];
} Pattern;

typedef struct Sample {
    char name[TRACKER_MAX_SAMPLE_NAME];
    char path[TRACKER_MAX_SAMPLE_PATH];
    float *data;
    size_t frames;
    int sample_rate;
    int channels;
} Sample;

typedef struct ChannelState {
    Sample samples[TRACKER_MAX_SAMPLES_PER_CHANNEL];
    int sample_count;
    int selected_sample;
    float level;
    bool mute;
} ChannelState;

typedef struct TrackerProject {
    int bpm;
    int swing;
    int current_pattern;
    int playing_step;
    char project_name[128];
    Pattern pattern;
    ChannelState channels[TRACKER_CHANNELS];
} TrackerProject;

static inline void tracker_project_init(TrackerProject *project) {
    if (!project) {
        return;
    }

    project->bpm = 125;
    project->swing = 0;
    project->current_pattern = 0;
    project->playing_step = 0;
    project->project_name[0] = '\0';

    for (int ch = 0; ch < TRACKER_CHANNELS; ++ch) {
        project->channels[ch].sample_count = 0;
        project->channels[ch].selected_sample = 0;
        project->channels[ch].level = 1.0f;
        project->channels[ch].mute = false;
        for (int step = 0; step < TRACKER_STEPS; ++step) {
            StepEvent *ev = &project->pattern.steps[ch][step];
            ev->active = false;
            ev->sample_index = 0;
            ev->note = 0;
            ev->volume = 1.0f;
            ev->effect = STEP_EFFECT_NONE;
            ev->effect_amount = 0.0f;
        }
    }
}

#endif
