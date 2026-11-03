#include "timelapse_cfg.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TL_USB "/mnt/exUDISK/"

static int has_dotdot(const char *path) {
    return path && (strstr(path, "..") != NULL || strstr(path, "//") != NULL);
}

static int chars_ok(const char *path) {
    for (const unsigned char *p = (const unsigned char *)path; *p; ++p) {
        if (isalnum(*p) || *p == '/' || *p == '_' || *p == '-' || *p == '.') continue;
        return 0;
    }
    return 1;
}

int tl_cfg_path_ok(const char *path, const char *prefix, size_t max_len) {
    size_t n = strlen(prefix);
    return path && strlen(path) < max_len && strncmp(path, prefix, n) == 0 && path[n] != '\0';
}

int tl_cfg_storage_ok(const char *path, int temp_link) {
    if (!path || !*path || strlen(path) >= TL_CFG_PATH_MAX || !chars_ok(path) || has_dotdot(path)) return 0;
    if (temp_link) return tl_cfg_path_ok(path, "/tmp/", TL_CFG_PATH_MAX) || tl_cfg_path_ok(path, "/opt/usr/", TL_CFG_PATH_MAX);
    if (tl_cfg_path_ok(path, "/opt/usr/", TL_CFG_PATH_MAX)) return 1;
    if (strncmp(path, TL_USB, strlen(TL_USB)) == 0 && path[strlen(TL_USB)]) return 1;
    return 0;
}

void tl_cfg_defaults(tl_cfg *out) {
    *out = (tl_cfg){0};
    snprintf(out->recording_mode, sizeof(out->recording_mode), "layer_timelapse");
    out->width = 640;
    out->height = 360;
    out->camera_fps = 20;
    out->output_fps = 30;
    out->bit_rate = 4000000;
    out->crf = 20;
    snprintf(out->preset, sizeof(out->preset), "medium");
    snprintf(out->profile, sizeof(out->profile), "main");
    snprintf(out->level, sizeof(out->level), "3.1");
    snprintf(out->tune, sizeof(out->tune), "stillimage");
    out->pic_max_size = 307200;
    out->keep_source_jpegs = 0;
    snprintf(out->picture_dir, sizeof(out->picture_dir), "/opt/usr/picture/");
    snprintf(out->video_dir, sizeof(out->video_dir), "/opt/usr/video/");
    snprintf(out->temp_link, sizeof(out->temp_link), "/tmp/pic_link");
}

static int parse_bool(const char *value) {
    if (!value) return 0;
    while (isspace((unsigned char)*value)) value++;
    return !strcasecmp(value, "true") || !strcasecmp(value, "True") || value[0] == '1';
}

static int parse_line(tl_cfg *s, char *line) {
    char *p = line;
    while (isspace((unsigned char)*p)) p++;
    if (*p == '#' || *p == ';' || !*p) return 0;
    char *colon = strchr(p, ':');
    if (!colon) return 0;
    *colon = 0;
    char *val = colon + 1;
    while (*val == ' ') val++;
    char *end = val + strlen(val);
    while (end > val && isspace((unsigned char)end[-1])) *--end = 0;
    if (!strcmp(p, "recording_mode")) snprintf(s->recording_mode, sizeof(s->recording_mode), "%.31s", val);
    else if (!strcmp(p, "width")) s->width = atoi(val);
    else if (!strcmp(p, "height")) s->height = atoi(val);
    else if (!strcmp(p, "camera_fps")) s->camera_fps = atoi(val);
    else if (!strcmp(p, "output_fps")) s->output_fps = atoi(val);
    else if (!strcmp(p, "bit_rate")) s->bit_rate = atoi(val);
    else if (!strcmp(p, "crf")) s->crf = atoi(val);
    else if (!strcmp(p, "preset")) snprintf(s->preset, sizeof(s->preset), "%.23s", val);
    else if (!strcmp(p, "profile")) snprintf(s->profile, sizeof(s->profile), "%.15s", val);
    else if (!strcmp(p, "level")) snprintf(s->level, sizeof(s->level), "%.7s", val);
    else if (!strcmp(p, "tune")) snprintf(s->tune, sizeof(s->tune), "%.23s", val);
    else if (!strcmp(p, "pic_max_size")) s->pic_max_size = atoi(val);
    else if (!strcmp(p, "keep_source_jpegs")) s->keep_source_jpegs = parse_bool(val);
    else if (!strcmp(p, "picture_dir")) snprintf(s->picture_dir, sizeof(s->picture_dir), "%.127s", val);
    else if (!strcmp(p, "video_dir")) snprintf(s->video_dir, sizeof(s->video_dir), "%.127s", val);
    else if (!strcmp(p, "temp_link")) snprintf(s->temp_link, sizeof(s->temp_link), "%.127s", val);
    else return 0;
    return 1;
}

static int validate(const tl_cfg *s, char *error, size_t cap) {
    if (!s->recording_mode[0] || (strcmp(s->recording_mode, "layer_timelapse") && strcmp(s->recording_mode, "realtime_timelapse"))) {
        snprintf(error, cap, "Invalid recording_mode");
        return 0;
    }
    if (s->width < 320 || s->width > 1920 || (s->width & 1) || s->height < 240 || s->height > 1080 || (s->height & 1)) {
        snprintf(error, cap, "Invalid width or height");
        return 0;
    }
    if (!tl_cfg_storage_ok(s->picture_dir, 0) || !tl_cfg_storage_ok(s->video_dir, 0) || !tl_cfg_storage_ok(s->temp_link, 1)) {
        snprintf(error, cap, "Invalid storage path");
        return 0;
    }
    error[0] = 0;
    return 1;
}

int tl_cfg_load(const char *path, tl_cfg *out, char *error, size_t error_cap) {
    tl_cfg_defaults(out);
    FILE *f = fopen(path, "r");
    if (!f) {
        if (errno == ENOENT) return 1;
        snprintf(error, error_cap, "Cannot open cfg");
        return 0;
    }
    char line[512];
    int section = 0, seen = 0;
    while (fgets(line, sizeof(line), f)) {
        char *p = line;
        while (isspace((unsigned char)*p)) p++;
        if (*p == '[') {
            char name[32];
            section = sscanf(p, "[%31[^]]]", name) == 1 && !strcmp(name, "timelapse");
            continue;
        }
        if (section && parse_line(out, line)) seen = 1;
    }
    int ok = !ferror(f);
    fclose(f);
    if (!ok) {
        snprintf(error, error_cap, "Read error");
        return 0;
    }
    if (seen && !validate(out, error, error_cap)) return 0;
    return 1;
}

#ifdef TL_CFG_TEST_MAIN
int main(int argc, char **argv) {
    tl_cfg cfg;
    char err[128];
    const char *path = argc > 1 ? argv[1] : "/opt/usr/cfg/timelapse.cfg";
    if (!tl_cfg_load(path, &cfg, err, sizeof(err))) {
        fprintf(stderr, "FAIL: %s\n", err);
        return 1;
    }
    printf("OK width=%d height=%d bit_rate=%d picture_dir=%s\n", cfg.width, cfg.height, cfg.bit_rate, cfg.picture_dir);
    return 0;
}
#endif
