#ifndef TUI_H
#define TUI_H

#include "engine.h"
#include "tracker_state.h"

int tui_run(TrackerProject *project, EngineState *engine, const char *sample_root);

#endif
