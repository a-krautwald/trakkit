#ifndef PROJECT_IO_H
#define PROJECT_IO_H

#include "tracker_state.h"

int project_save(const TrackerProject *project, const char *path);
int project_load(TrackerProject *project, const char *path);

#endif
