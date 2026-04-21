#include "project_io.h"

#include <stdio.h>
#include <string.h>

int project_save(const TrackerProject *project, const char *path) {
    if (!project || !path) {
        return -1;
    }

    FILE *f = fopen(path, "w");
    if (!f) {
        return -1;
    }

    fprintf(f, "TRAKKIT1\n");
    fprintf(f, "bpm=%d\n", project->bpm);
    fprintf(f, "swing=%d\n", project->swing);
    fprintf(f, "name=%s\n", project->project_name);

    for (int ch = 0; ch < TRACKER_CHANNELS; ++ch) {
        fprintf(f, "channel=%d selected=%d level=%.4f mute=%d\n",
                ch,
                project->channels[ch].selected_sample,
                project->channels[ch].level,
                project->channels[ch].mute ? 1 : 0);
    }

    for (int ch = 0; ch < TRACKER_CHANNELS; ++ch) {
        for (int step = 0; step < TRACKER_STEPS; ++step) {
            const StepEvent *ev = &project->pattern.steps[ch][step];
            if (!ev->active) {
                continue;
            }
            fprintf(f,
                    "step ch=%d idx=%d sample=%d note=%d vol=%.4f fx=%d fxamt=%.4f\n",
                    ch,
                    step,
                    ev->sample_index,
                    ev->note,
                    ev->volume,
                    (int)ev->effect,
                    ev->effect_amount);
        }
    }
    fclose(f);
    return 0;
}

int project_load(TrackerProject *project, const char *path) {
    if (!project || !path) {
        return -1;
    }

    FILE *f = fopen(path, "r");
    if (!f) {
        return -1;
    }

    project->bpm = 125;
    project->swing = 0;
    project->current_pattern = 0;
    project->playing_step = 0;
    project->project_name[0] = '\0';
    for (int ch = 0; ch < TRACKER_CHANNELS; ++ch) {
        project->channels[ch].selected_sample = 0;
        project->channels[ch].level = 1.0f;
        project->channels[ch].mute = 0;
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
    char line[512];
    if (!fgets(line, sizeof(line), f)) {
        fclose(f);
        return -1;
    }
    if (strncmp(line, "TRAKKIT1", 8) != 0) {
        fclose(f);
        return -1;
    }

    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "bpm=", 4) == 0) {
            sscanf(line + 4, "%d", &project->bpm);
        } else if (strncmp(line, "swing=", 6) == 0) {
            sscanf(line + 6, "%d", &project->swing);
        } else if (strncmp(line, "name=", 5) == 0) {
            strncpy(project->project_name, line + 5, sizeof(project->project_name) - 1);
            project->project_name[sizeof(project->project_name) - 1] = '\0';
            size_t len = strlen(project->project_name);
            if (len > 0 && project->project_name[len - 1] == '\n') {
                project->project_name[len - 1] = '\0';
            }
        } else if (strncmp(line, "channel=", 8) == 0) {
            int ch = 0;
            int selected = 0;
            float level = 1.0f;
            int mute = 0;
            if (sscanf(line, "channel=%d selected=%d level=%f mute=%d", &ch, &selected, &level, &mute) == 4) {
                if (ch >= 0 && ch < TRACKER_CHANNELS) {
                    project->channels[ch].selected_sample = selected;
                    project->channels[ch].level = level;
                    project->channels[ch].mute = mute != 0;
                }
            }
        } else if (strncmp(line, "step ", 5) == 0) {
            int ch = 0, idx = 0, sample = 0, note = 0, fx = 0;
            float vol = 1.0f, fxamt = 0.0f;
            if (sscanf(line,
                       "step ch=%d idx=%d sample=%d note=%d vol=%f fx=%d fxamt=%f",
                       &ch,
                       &idx,
                       &sample,
                       &note,
                       &vol,
                       &fx,
                       &fxamt) == 7) {
                if (ch >= 0 && ch < TRACKER_CHANNELS && idx >= 0 && idx < TRACKER_STEPS) {
                    StepEvent *ev = &project->pattern.steps[ch][idx];
                    ev->active = true;
                    ev->sample_index = sample;
                    ev->note = note;
                    ev->volume = vol;
                    ev->effect = (StepEffectType)fx;
                    ev->effect_amount = fxamt;
                }
            }
        }
    }
    fclose(f);
    return 0;
}
