#include "timelapse_path.h"
#include <string.h>

int timelapse_path_ok(const char *path, const char *allowed_prefix, size_t max_len) {
    if (!path || !allowed_prefix) return 0;
    size_t n = strlen(allowed_prefix);
    return strlen(path) < max_len && strncmp(path, allowed_prefix, n) == 0 && path[n] != '\0' && !strstr(path, "..");
}
