#pragma once
#include "timelapse_quality.h"

#ifdef __cplusplus
extern "C" {
#endif

int timelapse_path_ok(const char *path, const char *allowed_prefix, size_t max_len);

#ifdef __cplusplus
}
#endif
