#include "sample_bank.h"

#include <dirent.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

typedef struct WavHeader {
    char riff[4];
    uint32_t file_size;
    char wave[4];
    char fmt_id[4];
    uint32_t fmt_size;
    uint16_t audio_format;
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
} WavHeader;

static int ends_with_wav(const char *name) {
    size_t len = strlen(name);
    if (len < 4) {
        return 0;
    }
    const char *ext = &name[len - 4];
    return strcasecmp(ext, ".wav") == 0;
}

static int load_wav_file(Sample *sample, const char *path, const char *name) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        return -1;
    }

    WavHeader h;
    if (fread(&h, sizeof(WavHeader), 1, f) != 1) {
        fclose(f);
        return -1;
    }

    if (memcmp(h.riff, "RIFF", 4) != 0 || memcmp(h.wave, "WAVE", 4) != 0) {
        fclose(f);
        return -1;
    }
    if (h.audio_format != 1 || h.bits_per_sample != 16) {
        fclose(f);
        return -1;
    }

    char chunk_id[4];
    uint32_t chunk_size = 0;
    int found_data = 0;
    while (fread(chunk_id, 1, 4, f) == 4 && fread(&chunk_size, 4, 1, f) == 1) {
        if (memcmp(chunk_id, "data", 4) == 0) {
            found_data = 1;
            break;
        }
        fseek(f, (long)chunk_size, SEEK_CUR);
    }
    if (!found_data) {
        fclose(f);
        return -1;
    }

    size_t sample_count = chunk_size / sizeof(int16_t);
    int16_t *raw = (int16_t *)malloc(sample_count * sizeof(int16_t));
    if (!raw) {
        fclose(f);
        return -1;
    }
    if (fread(raw, sizeof(int16_t), sample_count, f) != sample_count) {
        free(raw);
        fclose(f);
        return -1;
    }
    fclose(f);

    sample->channels = h.num_channels;
    sample->sample_rate = (int)h.sample_rate;
    sample->frames = sample_count / h.num_channels;
    sample->data = (float *)malloc(sample->frames * h.num_channels * sizeof(float));
    if (!sample->data) {
        free(raw);
        return -1;
    }
    for (size_t i = 0; i < sample_count; ++i) {
        sample->data[i] = (float)raw[i] / 32768.0f;
    }
    free(raw);

    snprintf(sample->name, sizeof(sample->name), "%s", name);
    snprintf(sample->path, sizeof(sample->path), "%s", path);
    return 0;
}

int sample_bank_load_channels(TrackerProject *project, const char *base_path) {
    if (!project || !base_path) {
        return -1;
    }

    sample_bank_free(project);

    int total = 0;
    char channel_dir[TRACKER_MAX_SAMPLE_PATH];
    for (int ch = 0; ch < TRACKER_CHANNELS; ++ch) {
        snprintf(channel_dir, sizeof(channel_dir), "%s/%d", base_path, ch + 1);
        DIR *dir = opendir(channel_dir);
        if (!dir) {
            continue;
        }

        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_name[0] == '.') {
                continue;
            }
            if (!ends_with_wav(entry->d_name)) {
                continue;
            }

            ChannelState *channel = &project->channels[ch];
            if (channel->sample_count >= TRACKER_MAX_SAMPLES_PER_CHANNEL) {
                break;
            }

            char full_path[TRACKER_MAX_SAMPLE_PATH];
            int n = snprintf(full_path, sizeof(full_path), "%s/%s", channel_dir, entry->d_name);
            if (n < 0 || (size_t)n >= sizeof(full_path)) {
                continue;
            }
            Sample *slot = &channel->samples[channel->sample_count];
            if (load_wav_file(slot, full_path, entry->d_name) == 0) {
                channel->sample_count++;
                total++;
            }
        }
        closedir(dir);
    }

    return total;
}

void sample_bank_free(TrackerProject *project) {
    if (!project) {
        return;
    }
    for (int ch = 0; ch < TRACKER_CHANNELS; ++ch) {
        ChannelState *channel = &project->channels[ch];
        for (int i = 0; i < channel->sample_count; ++i) {
            free(channel->samples[i].data);
            channel->samples[i].data = NULL;
            channel->samples[i].frames = 0;
        }
        channel->sample_count = 0;
    }
}

const char *sample_bank_effect_name(StepEffectType effect) {
    switch (effect) {
        case STEP_EFFECT_DISTORTION: return "DIST";
        case STEP_EFFECT_LOWPASS: return "LPF";
        case STEP_EFFECT_HIGHPASS: return "HPF";
        case STEP_EFFECT_REVERB: return "RVB";
        case STEP_EFFECT_NONE:
        default:
            return "---";
    }
}
