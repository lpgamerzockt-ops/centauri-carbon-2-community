#include <limits.h>
#include <stdlib.h>

#ifndef CC2_TIMELAPSE_SETTINGS_H
#define CC2_TIMELAPSE_SETTINGS_H
/* Persistent timelapse/video quality settings for Phase 3 firmware.
 * CC2 Control reads/writes /opt/usr/cfg/timelapse.cfg (Klipper-style section).
 * Stock ai_camera does not consume this file until a custom firmware build wires it. */
#define TIMELAPSE_CFG_MAX 8192
#define TIMELAPSE_PATH_MAX 128

typedef struct {
    char recording_mode[32];
    int width, height, camera_fps, output_fps;
    int bit_rate, crf, pic_max_size;
    char preset[24], profile[16], level[8], tune[24];
    int keep_source_jpegs;
    char picture_dir[TIMELAPSE_PATH_MAX], video_dir[TIMELAPSE_PATH_MAX], temp_link[TIMELAPSE_PATH_MAX];
} timelapse_settings;

extern const char *cc2_timelapse_settings_file;

static void timelapse_defaults(timelapse_settings *s) {
    *s = (timelapse_settings){0};
    snprintf(s->recording_mode, sizeof(s->recording_mode), "layer_timelapse");
    s->width = 640;
    s->height = 360;
    s->camera_fps = 20;
    s->output_fps = 30;
    s->bit_rate = 4000000;
    s->crf = 20;
    snprintf(s->preset, sizeof(s->preset), "medium");
    snprintf(s->profile, sizeof(s->profile), "main");
    snprintf(s->level, sizeof(s->level), "3.1");
    snprintf(s->tune, sizeof(s->tune), "stillimage");
    s->pic_max_size = 307200;
    s->keep_source_jpegs = 0;
    snprintf(s->picture_dir, sizeof(s->picture_dir), "/opt/usr/picture/");
    snprintf(s->video_dir, sizeof(s->video_dir), "/opt/usr/video/");
    snprintf(s->temp_link, sizeof(s->temp_link), "/tmp/pic_link");
}

static int timelapse_has_dotdot(const char *path) {
    return path && (strstr(path, "..") != NULL || strstr(path, "//") != NULL);
}

static int timelapse_path_chars_ok(const char *path) {
    for (const unsigned char *p = (const unsigned char *)path; *p; ++p) {
        if (isalnum(*p) || *p == '/' || *p == '_' || *p == '-' || *p == '.') continue;
        return 0;
    }
    return 1;
}

static int timelapse_prefix_ok(const char *path, const char *prefix) {
    size_t n = strlen(prefix);
    return path && strncmp(path, prefix, n) == 0 && path[n] != '\0';
}

static int timelapse_validate_storage_path(const char *path, int temp_link) {
    if (!path || !*path || strlen(path) >= TIMELAPSE_PATH_MAX) return 0;
    if (!timelapse_path_chars_ok(path) || timelapse_has_dotdot(path)) return 0;
    if (temp_link) return timelapse_prefix_ok(path, "/tmp/") || timelapse_prefix_ok(path, "/opt/usr/");
    return timelapse_prefix_ok(path, "/opt/usr/");
}

static int timelapse_mode_ok(const char *mode) {
    return mode && (!strcmp(mode, "layer_timelapse") || !strcmp(mode, "realtime_timelapse"));
}

static int timelapse_preset_ok(const char *preset) {
    static const char *allowed[] = {"ultrafast", "superfast", "veryfast", "faster", "fast",
                                    "medium", "slow", "slower", "veryslow", NULL};
    if (!preset || !*preset) return 0;
    for (int i = 0; allowed[i]; ++i)
        if (!strcmp(preset, allowed[i])) return 1;
    return 0;
}

static int timelapse_profile_ok(const char *profile) {
    return profile && (!strcmp(profile, "baseline") || !strcmp(profile, "main") || !strcmp(profile, "high"));
}

static int timelapse_validate(const timelapse_settings *s, char *error, size_t cap) {
    if (!timelapse_mode_ok(s->recording_mode)) {
        snprintf(error, cap, "Invalid recording_mode");
        return 0;
    }
    if (s->width < 320 || s->width > 1920 || (s->width & 1) || s->height < 240 || s->height > 1080 || (s->height & 1)) {
        snprintf(error, cap, "Invalid width or height");
        return 0;
    }
    if (s->camera_fps < 5 || s->camera_fps > 30 || s->output_fps < 10 || s->output_fps > 60) {
        snprintf(error, cap, "Invalid camera_fps or output_fps");
        return 0;
    }
    if (s->bit_rate < 500000 || s->bit_rate > 20000000 || s->crf < 0 || s->crf > 51) {
        snprintf(error, cap, "Invalid bit_rate or crf");
        return 0;
    }
    if (!timelapse_preset_ok(s->preset) || !timelapse_profile_ok(s->profile) || !s->level[0]) {
        snprintf(error, cap, "Invalid encoder preset, profile or level");
        return 0;
    }
    if (s->pic_max_size < 102400 || s->pic_max_size > 1048576) {
        snprintf(error, cap, "Invalid pic_max_size");
        return 0;
    }
    if (!timelapse_validate_storage_path(s->picture_dir, 0) || !timelapse_validate_storage_path(s->video_dir, 0) ||
        !timelapse_validate_storage_path(s->temp_link, 1)) {
        snprintf(error, cap, "Invalid storage path");
        return 0;
    }
    error[0] = 0;
    return 1;
}

static int timelapse_parse_bool(const char *value) {
    if (!value) return 0;
    while (isspace((unsigned char)*value)) value++;
    return !strcasecmp(value, "true") || !strcasecmp(value, "True") || value[0] == '1';
}

static int timelapse_parse_line(timelapse_settings *s, char *line) {
    char *p = line;
    while (isspace((unsigned char)*p)) p++;
    if (*p == '#' || *p == ';' || !*p) return 0;
    char *colon = strchr(p, ':');
    if (!colon) return 0;
    *colon = 0;
    char *key = p;
    char *val = colon + 1;
    while (*val == ' ') val++;
    char *end = val + strlen(val);
    while (end > val && isspace((unsigned char)end[-1])) *--end = 0;
    if (!strcmp(key, "recording_mode")) snprintf(s->recording_mode, sizeof(s->recording_mode), "%.31s", val);
    else if (!strcmp(key, "width")) s->width = atoi(val);
    else if (!strcmp(key, "height")) s->height = atoi(val);
    else if (!strcmp(key, "camera_fps")) s->camera_fps = atoi(val);
    else if (!strcmp(key, "output_fps")) s->output_fps = atoi(val);
    else if (!strcmp(key, "bit_rate")) s->bit_rate = atoi(val);
    else if (!strcmp(key, "crf")) s->crf = atoi(val);
    else if (!strcmp(key, "preset")) snprintf(s->preset, sizeof(s->preset), "%.23s", val);
    else if (!strcmp(key, "profile")) snprintf(s->profile, sizeof(s->profile), "%.15s", val);
    else if (!strcmp(key, "level")) snprintf(s->level, sizeof(s->level), "%.7s", val);
    else if (!strcmp(key, "tune")) snprintf(s->tune, sizeof(s->tune), "%.23s", val);
    else if (!strcmp(key, "pic_max_size")) s->pic_max_size = atoi(val);
    else if (!strcmp(key, "keep_source_jpegs")) s->keep_source_jpegs = timelapse_parse_bool(val);
    else if (!strcmp(key, "picture_dir")) snprintf(s->picture_dir, sizeof(s->picture_dir), "%.127s", val);
    else if (!strcmp(key, "video_dir")) snprintf(s->video_dir, sizeof(s->video_dir), "%.127s", val);
    else if (!strcmp(key, "temp_link")) snprintf(s->temp_link, sizeof(s->temp_link), "%.127s", val);
    return 1;
}

static int timelapse_read_file(timelapse_settings *out) {
    timelapse_defaults(out);
    FILE *f = fopen(cc2_timelapse_settings_file, "r");
    if (!f) return errno == ENOENT ? 1 : 0;
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
        if (section && timelapse_parse_line(out, line)) seen = 1;
    }
    int ok = !ferror(f);
    fclose(f);
    if (!ok) return 0;
    char err[80];
    if (seen && !timelapse_validate(out, err, sizeof(err))) return 0;
    return 1;
}

static int timelapse_write_file(const timelapse_settings *s) {
    char err[80];
    if (!timelapse_validate(s, err, sizeof(err))) return 0;
    char body[TIMELAPSE_CFG_MAX];
    int n = snprintf(body, sizeof(body),
        "# CC2 Control timelapse settings (requires Phase 3 ai_camera to apply)\n"
        "[timelapse]\n"
        "recording_mode: %s\n"
        "width: %d\n"
        "height: %d\n"
        "camera_fps: %d\n"
        "output_fps: %d\n"
        "bit_rate: %d\n"
        "crf: %d\n"
        "preset: %s\n"
        "tune: %s\n"
        "profile: %s\n"
        "level: %s\n"
        "pic_max_size: %d\n"
        "keep_source_jpegs: %s\n"
        "picture_dir: %s\n"
        "video_dir: %s\n"
        "temp_link: %s\n",
        s->recording_mode, s->width, s->height, s->camera_fps, s->output_fps, s->bit_rate, s->crf, s->preset, s->tune,
        s->profile, s->level, s->pic_max_size, s->keep_source_jpegs ? "True" : "False", s->picture_dir, s->video_dir,
        s->temp_link);
    if (n <= 0 || (size_t)n >= sizeof(body)) return 0;
    char dir[] = "/opt/usr/cfg";
    struct stat st;
    if (stat(dir, &st) != 0 && mkdir(dir, 0755) != 0 && errno != EEXIST) return 0;
    char temp[TIMELAPSE_PATH_MAX + 16];
    if (snprintf(temp, sizeof(temp), "%s.new", cc2_timelapse_settings_file) >= (int)sizeof(temp)) return 0;
    int fd = open(temp, O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW, 0644);
    if (fd < 0) return 0;
    int ok = write(fd, body, (size_t)n) == n;
    if (fsync(fd)) ok = 0;
    if (close(fd)) ok = 0;
    if (ok && rename(temp, cc2_timelapse_settings_file)) ok = 0;
    if (!ok) unlink(temp);
    return ok;
}

static void timelapse_json_escape(const char *in, char *out, size_t cap) {
    size_t used = 0;
    for (const unsigned char *p = (const unsigned char *)in; *p && used + 2 < cap; ++p) {
        if (*p == '"' || *p == '\\') {
            out[used++] = '\\';
            out[used++] = (char)*p;
        } else if (*p >= 32) out[used++] = (char)*p;
    }
    out[used] = 0;
}

static void timelapse_settings_get(int fd) {
    timelapse_settings s;
    int loaded = timelapse_read_file(&s);
    char rm[64], pd[160], vd[160], tl[160], pr[32], pf[20], lv[12], tu[32];
    timelapse_json_escape(s.recording_mode, rm, sizeof(rm));
    timelapse_json_escape(s.picture_dir, pd, sizeof(pd));
    timelapse_json_escape(s.video_dir, vd, sizeof(vd));
    timelapse_json_escape(s.temp_link, tl, sizeof(tl));
    timelapse_json_escape(s.preset, pr, sizeof(pr));
    timelapse_json_escape(s.profile, pf, sizeof(pf));
    timelapse_json_escape(s.level, lv, sizeof(lv));
    timelapse_json_escape(s.tune, tu, sizeof(tu));
    char body[2048];
    int n = snprintf(body, sizeof(body),
        "{\"loaded\":%s,\"firmware_support\":false,\"path\":\"%s\","
        "\"recording_mode\":\"%s\",\"width\":%d,\"height\":%d,\"camera_fps\":%d,\"output_fps\":%d,"
        "\"bit_rate\":%d,\"crf\":%d,\"preset\":\"%s\",\"tune\":\"%s\",\"profile\":\"%s\",\"level\":\"%s\","
        "\"pic_max_size\":%d,\"keep_source_jpegs\":%s,"
        "\"picture_dir\":\"%s\",\"video_dir\":\"%s\",\"temp_link\":\"%s\"}\n",
        loaded ? "true" : "false", cc2_timelapse_settings_file, rm, s.width, s.height, s.camera_fps, s.output_fps,
        s.bit_rate, s.crf, pr, tu, pf, lv, s.pic_max_size, s.keep_source_jpegs ? "true" : "false", pd, vd, tl);
    respond(fd, loaded ? 200 : 503, loaded ? "OK" : "Service Unavailable", "application/json; charset=utf-8", body,
            (size_t)(n > 0 ? n : 0));
}

static int timelapse_json_bool(const char *body, size_t len, const char *key, int *out) {
    char pattern[64];
    snprintf(pattern, sizeof(pattern), "\"%s\":", key);
    const char *p = strstr(body, pattern);
    if (!p || (size_t)(p - body) >= len) return 0;
    p += strlen(pattern);
    while (p < body + len && isspace((unsigned char)*p)) p++;
    if (p >= body + len) return 0;
    if (!strncmp(p, "true", 4)) {
        *out = 1;
        return 1;
    }
    if (!strncmp(p, "false", 5)) {
        *out = 0;
        return 1;
    }
    return 0;
}

static int timelapse_json_int(const char *body, size_t len, const char *key, int *out) {
    char pattern[64];
    snprintf(pattern, sizeof(pattern), "\"%s\":", key);
    const char *p = strstr(body, pattern);
    if (!p || (size_t)(p - body) >= len) return 0;
    p += strlen(pattern);
    while (p < body + len && isspace((unsigned char)*p)) p++;
    if (p >= body + len || (*p != '-' && !isdigit((unsigned char)*p))) return 0;
    char *end;
    long v = strtol(p, &end, 10);
    if (end == p || v < INT_MIN || v > INT_MAX) return 0;
    *out = (int)v;
    return 1;
}

static int timelapse_json_string(const char *body, size_t len, const char *key, char *out, size_t cap) {
    char pattern[64];
    snprintf(pattern, sizeof(pattern), "\"%s\":\"", key);
    const char *p = strstr(body, pattern);
    if (!p || (size_t)(p - body) >= len) return 0;
    p += strlen(pattern);
    size_t used = 0;
    while (p < body + len && *p && used + 1 < cap) {
        if (*p == '\\' && p + 1 < body + len) {
            p++;
            out[used++] = *p++;
            continue;
        }
        if (*p == '"') {
            out[used] = 0;
            return 1;
        }
        out[used++] = *p++;
    }
    return 0;
}

static void timelapse_settings_post(int fd, const mqtt_client *mqtt, const char *body, size_t body_len) {
    if (!gcode_idle_for_mutation(mqtt)) {
        const char *error = "{\"error\":\"Printer must be idle to change timelapse settings\"}\n";
        respond(fd, 409, "Conflict", "application/json; charset=utf-8", error, strlen(error));
        return;
    }
    timelapse_settings s;
    timelapse_read_file(&s);
    int v;
    char text[TIMELAPSE_PATH_MAX];
    if (timelapse_json_string(body, body_len, "recording_mode", text, sizeof(text))) snprintf(s.recording_mode, sizeof(s.recording_mode), "%.31s", text);
    if (timelapse_json_int(body, body_len, "width", &v)) s.width = v;
    if (timelapse_json_int(body, body_len, "height", &v)) s.height = v;
    if (timelapse_json_int(body, body_len, "camera_fps", &v)) s.camera_fps = v;
    if (timelapse_json_int(body, body_len, "output_fps", &v)) s.output_fps = v;
    if (timelapse_json_int(body, body_len, "bit_rate", &v)) s.bit_rate = v;
    if (timelapse_json_int(body, body_len, "crf", &v)) s.crf = v;
    if (timelapse_json_int(body, body_len, "pic_max_size", &v)) s.pic_max_size = v;
    if (timelapse_json_string(body, body_len, "preset", text, sizeof(text))) snprintf(s.preset, sizeof(s.preset), "%.23s", text);
    if (timelapse_json_string(body, body_len, "profile", text, sizeof(text))) snprintf(s.profile, sizeof(s.profile), "%.15s", text);
    if (timelapse_json_string(body, body_len, "level", text, sizeof(text))) snprintf(s.level, sizeof(s.level), "%.7s", text);
    if (timelapse_json_string(body, body_len, "tune", text, sizeof(text))) snprintf(s.tune, sizeof(s.tune), "%.23s", text);
    if (timelapse_json_string(body, body_len, "picture_dir", text, sizeof(text))) snprintf(s.picture_dir, sizeof(s.picture_dir), "%.127s", text);
    if (timelapse_json_string(body, body_len, "video_dir", text, sizeof(text))) snprintf(s.video_dir, sizeof(s.video_dir), "%.127s", text);
    if (timelapse_json_string(body, body_len, "temp_link", text, sizeof(text))) snprintf(s.temp_link, sizeof(s.temp_link), "%.127s", text);
    if (timelapse_json_bool(body, body_len, "keep_source_jpegs", &v)) s.keep_source_jpegs = v;
    char err[120];
    if (!timelapse_validate(&s, err, sizeof(err))) {
        char msg[256];
        int n = snprintf(msg, sizeof(msg), "{\"error\":\"%s\"}\n", err);
        respond(fd, 400, "Bad Request", "application/json; charset=utf-8", msg, (size_t)(n > 0 ? n : 0));
        return;
    }
    if (!timelapse_write_file(&s)) {
        const char *error = "{\"error\":\"Could not save timelapse settings\"}\n";
        respond(fd, 503, "Service Unavailable", "application/json; charset=utf-8", error, strlen(error));
        return;
    }
    const char *ok = "{\"saved\":true,\"firmware_support\":false}\n";
    respond(fd, 200, "OK", "application/json; charset=utf-8", ok, strlen(ok));
}

#endif
