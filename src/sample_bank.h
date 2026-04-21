#ifndef SAMPLE_BANK_H
#define SAMPLE_BANK_H

#include "tracker_state.h"

int sample_bank_load_channels(TrackerProject *project, const char *base_path);
void sample_bank_free(TrackerProject *project);
const char *sample_bank_effect_name(StepEffectType effect);

#endif
