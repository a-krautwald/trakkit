#ifndef ENGINE_H
#define ENGINE_H

#include <stdbool.h>
#include "tracker_state.h"

typedef struct EngineState EngineState;

EngineState *engine_create(TrackerProject *project);
void engine_destroy(EngineState *engine);
int engine_start(EngineState *engine);
void engine_stop(EngineState *engine);
void engine_set_playing(EngineState *engine, bool playing);
bool engine_is_playing(const EngineState *engine);
void engine_lock(EngineState *engine);
void engine_unlock(EngineState *engine);

#endif
