# Building ai_camera with Murin timelapse (Phase 3)

This repository ships **CC2 Control** and an **overlay sketch**, not a rebuilt
`/opt/bin/ai_camera` and not the ELEGOO source tree.

A CC2 Control `.cc2update` or SSH overlay (Murin 1.2.25) **does not replace**
vendor `ai_camera`. Encoder quality (`bit_rate`, `crf`, `preset`) and USB
`picture_dir` / `video_dir` take effect only after a rebuilt camera binary is
installed inside **full Community Firmware**.

| Path | Purpose |
| --- | --- |
| `timelapse_cfg.c` / `timelapse_cfg.h` | Parser for `/opt/usr/cfg/timelapse.cfg` (same format as CC2) |
| `overlay/module/camera/*` | Runtime loader + path helper copied into vendor `ai_camera` |
| `apply_overlay.py` | Copies parser and overlay sources; does **not** rebuild firmware |
| `experimental/timelapse-phase3-ai_camera-sketch.patch` | Encoder/path hunks for vendor files |

The firmware builder (`builder/current/core/firmware_builder.py`) still requires
the qualified coexistence camera SHA (`CAMERA_RELEASE_SHA256` /
`ec5633b17de329d5b18c3f8d8de6ebf389391a04193536d976ba167022d07b99`) and **does
not** apply this overlay.

## Prerequisites (not in Git)

1. Official ELEGOO **CentauriCarbon2** `ai_camera` sources (vendor tree).
2. Vendor ARM toolchain for the printer SoC.
3. To produce a **flashable** image: official OTA
   `cc2_eeb001_02.01.01.07_20260922210235.zip.sig`, AES key, community private
   signing key `keys/cc2_community_release_private.pem`, SquashFS 4.6.1 tools,
   Connections runtime, Dual Trust payload, sshd component. Missing keys are a
   hard stop — do not generate placeholder signatures.
4. After a rebuilt `ai_camera`, update `CAMERA_RELEASE_SHA256` only to the
   **measured** hash of that binary (never a guessed value).

## Overlay onto vendor sources

```sh
export ELEGOO_AI_CAMERA_SRC=/path/to/elegoo/extras/ai_camera
cd builder/current/components/ai_camera_timelapse
python3 apply_overlay.py "$ELEGOO_AI_CAMERA_SRC"
```

Then in the vendor build:

1. Add `module/camera/timelapse_cfg.c`, `timelapse_runtime.c`, `timelapse_path.c`
   to the ai_camera target.
2. Call `tl_runtime_load("/opt/usr/cfg/timelapse.cfg");` at startup (before camera init).
3. Apply remaining hunks from `experimental/timelapse-phase3-ai_camera-sketch.patch`
   (`video_encoder.cpp`, `time_lapse_photo.h`, `uvc_config_info.h`, `ai_camera.cpp`).
4. Gate **realtime_timelapse** on `tl_runtime_recording_mode()` **and** vendor
   `REAL_TIME_SAVE_VIDEO`.
5. Replace `/opt/bin/ai_camera` in a **signed** Community Firmware image.

USB / offline install of that image (user only, never remote flash): copy the
unchanged signed `.zip.sig` to the USB root, then printer
**Settings → Check for Updates → Offline Update**. Unsigned RootFS / SWU will
not pass Dual Trust.

## Host test (parser only)

```sh
gcc -DTL_CFG_TEST_MAIN -o /tmp/tl_cfg_test timelapse_cfg.c && /tmp/tl_cfg_test /path/to/timelapse.cfg
```

Or: `python3 -m unittest builder/current/tests/test_timelapse_cfg_parser.py`

## Keys read from `[timelapse]`

`recording_mode`, `width`, `height`, `camera_fps`, `output_fps`, `bit_rate`, `crf`, `preset`, `tune`, `profile`, `level`, `pic_max_size`, `keep_source_jpegs`, `picture_dir`, `video_dir`, `temp_link`.

USB paths must start with `/mnt/exUDISK/` plus a subfolder (same mount as G-code USB).
