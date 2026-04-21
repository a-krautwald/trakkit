#include <stdio.h>
#include <string.h>

#include "engine.h"
#include "sample_bank.h"
#include "tracker_state.h"
#include "tui.h"

int main(int argc, char **argv) {
    const char *sample_root = "samples";
    if (argc > 1) {
        sample_root = argv[1];
    }

    TrackerProject project;
    tracker_project_init(&project);
    snprintf(project.project_name, sizeof(project.project_name), "new_project");

    int sample_count = sample_bank_load_channels(&project, sample_root);
    printf("Loaded %d samples from %s/1..8\n", sample_count, sample_root);

    EngineState *engine = engine_create(&project);
    if (!engine) {
        fprintf(stderr, "Failed to create engine\n");
        sample_bank_free(&project);
        return 1;
    }
    if (engine_start(engine) != 0) {
        fprintf(stderr, "Failed to start ALSA engine\n");
        engine_destroy(engine);
        sample_bank_free(&project);
        return 1;
    }

    tui_run(&project, engine, sample_root);

    engine_stop(engine);
    engine_destroy(engine);
    sample_bank_free(&project);
    return 0;
}
