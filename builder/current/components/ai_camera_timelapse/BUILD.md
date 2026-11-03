# Building ai_camera with Murin timelapse (Phase 3)

This repository ships **CC2 Control** and **firmware builder integration**, not the full ELEGOO `ai_camera` source tree. Phase 3 adds:

| Path | Purpose |
| --- | --- |
| `timelapse_cfg.c` / `timelapse_cfg.h` | Parser for `/opt/usr/cfg/timelapse.cfg` (same format as CC2) |
| `overlay/module/camera/*` | Headers + runtime loader linked into ai_camera |
| `apply_overlay.py` | Copies overlay into your vendor checkout |

## Prerequisites

1. Official ELEGOO **CentauriCarbon2** `ai_camera` sources (private/vendor tree).
2. Toolchain used by ELEGOO for the printer SoC (same as vendor firmware build).
3. CC2 Control already writing `timelapse.cfg` (Murin `optimierung` branch).

## Steps

```sh
export ELEGOO_AI_CAMERA_SRC=/path/to/elegoo/extras/ai_camera
cd builder/current/components/ai_camera_timelapse
python3 apply_overlay.py "$ELEGOO_AI_CAMERA_SRC"
```

Then in the vendor build:

1. Add `module/camera/timelapse_cfg.c`, `timelapse_runtime.c`, `timelapse_path.c` to the ai_camera target.
2. Call `tl_runtime_load("/opt/usr/cfg/timelapse.cfg");` at startup (before camera init).
3. Include `timelapse_runtime.h` instead of hard-coded `uvc_config_info.h` sizes where patched.
4. Gate **realtime_timelapse** on `tl_runtime_recording_mode()` **and** vendor `REAL_TIME_SAVE_VIDEO` build flag.

Replace `/opt/bin/ai_camera` in the **full Community Firmware** image (not via CC2 Control `.cc2update` alone).

## Host test (parser only)

```sh
gcc -DTL_CFG_TEST_MAIN -o /tmp/tl_cfg_test timelapse_cfg.c && /tmp/tl_cfg_test /path/to/timelapse.cfg
```

Or: `python3 -m unittest builder/current/tests/test_timelapse_cfg_parser.py`

## Keys read from `[timelapse]`

`recording_mode`, `width`, `height`, `camera_fps`, `output_fps`, `bit_rate`, `crf`, `preset`, `tune`, `profile`, `level`, `pic_max_size`, `keep_source_jpegs`, `picture_dir`, `video_dir`, `temp_link`.

USB paths must start with `/mnt/exUDISK/` + subfolder (matches CC2 G-code USB mount).
