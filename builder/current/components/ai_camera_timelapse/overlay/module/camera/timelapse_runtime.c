#include "timelapse_runtime.h"
#include <string.h>

tl_cfg g_tl_runtime;

int tl_runtime_load(const char *path) {
    char err[128];
    return tl_cfg_load(path ? path : "/opt/usr/cfg/timelapse.cfg", &g_tl_runtime, err, sizeof(err));
}

int tl_runtime_recording_mode(void) {
    return strcmp(g_tl_runtime.recording_mode, "realtime_timelapse") == 0 ? TIMELAPSE_RECORDING_REALTIME
                                                                           : TIMELAPSE_RECORDING_LAYER;
}
