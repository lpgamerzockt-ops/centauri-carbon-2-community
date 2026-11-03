#pragma once
/* Runtime-loaded timelapse settings (Phase 3). Call tl_runtime_load() once at ai_camera startup. */
#include "timelapse_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif

extern tl_cfg g_tl_runtime;

int tl_runtime_load(const char *path);
#define TIMELAPSE_RECORDING_LAYER 0
#define TIMELAPSE_RECORDING_REALTIME 1
int tl_runtime_recording_mode(void);

#define TIMELAPSE_WIDTH (g_tl_runtime.width)
#define TIMELAPSE_HEIGHT (g_tl_runtime.height)
#define TIMELAPSE_CAMERA_FPS (g_tl_runtime.camera_fps)
#define TIMELAPSE_OUTPUT_FPS (g_tl_runtime.output_fps)
#define TIMELAPSE_BIT_RATE (g_tl_runtime.bit_rate)
#define TIMELAPSE_CRF (g_tl_runtime.crf)
#define TIMELAPSE_CRF_ENABLED 1
#define TIMELAPSE_X264_PRESET (g_tl_runtime.preset)
#define TIMELAPSE_X264_TUNE (g_tl_runtime.tune)
#define TIMELAPSE_X264_PROFILE (g_tl_runtime.profile)
#define TIMELAPSE_X264_LEVEL (g_tl_runtime.level)
#define TIMELAPSE_PIC_MAX_SIZE (g_tl_runtime.pic_max_size)
#define TIMELAPSE_KEEP_SOURCE_JPEGS (g_tl_runtime.keep_source_jpegs)
#define TIMELAPSE_PICTURE_DIR (g_tl_runtime.picture_dir)
#define TIMELAPSE_VIDEO_DIR (g_tl_runtime.video_dir)
#define TIMELAPSE_TEMP_LINK (g_tl_runtime.temp_link)

#ifdef __cplusplus
}
#endif
