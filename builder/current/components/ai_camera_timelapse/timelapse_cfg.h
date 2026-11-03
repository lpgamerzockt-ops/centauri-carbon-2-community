#ifndef CC2_TIMELAPSE_CFG_H
#define CC2_TIMELAPSE_CFG_H
/* Shared timelapse.cfg parser for CC2 Control and ai_camera Phase 3 builds.
 * File format matches cc2-control/src/timelapse_settings.h ([timelapse] section). */

#include <stddef.h>

#define TL_CFG_PATH_MAX 128

typedef struct {
    char recording_mode[32];
    int width, height, camera_fps, output_fps;
    int bit_rate, crf, pic_max_size;
    char preset[24], profile[16], level[8], tune[24];
    int keep_source_jpegs;
    char picture_dir[TL_CFG_PATH_MAX], video_dir[TL_CFG_PATH_MAX], temp_link[TL_CFG_PATH_MAX];
} tl_cfg;

#ifdef __cplusplus
extern "C" {
#endif

void tl_cfg_defaults(tl_cfg *out);
/* Returns 1 on success (missing file → defaults, errno ENOENT ok). 0 on parse/validation error. */
int tl_cfg_load(const char *path, tl_cfg *out, char *error, size_t error_cap);
int tl_cfg_path_ok(const char *path, const char *prefix, size_t max_len);
int tl_cfg_storage_ok(const char *path, int temp_link);

#ifdef __cplusplus
}
#endif

#endif
