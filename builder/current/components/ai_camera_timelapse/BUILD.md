# Building ai_camera with Murin timelapse (Phase 3)

This repository ships **CC2 Control** and an overlay sketch, **not** a complete
`ai_camera` binary. `builder/current/components/ai_camera_timelapse/` cannot
produce `/opt/bin/ai_camera` by itself.

| Path | Purpose |
| --- | --- |
| `timelapse_cfg.c` / `timelapse_cfg.h` | Parser for `/opt/usr/cfg/timelapse.cfg` (same format as CC2) |
| `overlay/module/camera/*` | Headers + runtime loader linked into vendor `ai_camera` |
| `apply_overlay.py` | Copies overlay into a vendor checkout |

## Cannot build here without

1. ELEGOO **CentauriCarbon2** tree: `elegoo/extras/ai_camera` **and** `elegoo/lib`
   (ARM `.so`: libhv, ffmpeg, libjpeg, curl, openssl, agora). Public dump:
   <https://github.com/elegooofficial/CentauriCarbon2> (example commit
   `5a2ea7fc`). Not vendored in this community repo.
2. Qualified printer toolchain **GCC 6.5.0 / glibc 2.23**, prefix
   `/opt/cc2-cross/toolchain-out/bin/arm-cortex_a15-linux-gnueabihf`
   (same as `builder/current/prepare_reactor.py`). Distro
   `gcc-arm-linux-gnueabihf` is not a substitute.
3. Manual encoder hunks: `experimental/timelapse-phase3-ai_camera-sketch.patch`.
4. CMake edits: add `timelapse_cfg.c`, `timelapse_runtime.c`, `timelapse_path.c`
   to the `ai_camera` target; call `tl_runtime_load("/opt/usr/cfg/timelapse.cfg")`
   at startup.

Public ELEGOO sources **predate** the camera binary on Community Firmware
(`docs/RELEASE_CANDIDATE.md`). A from-source rebuild also **does not** apply
the byte-pinned coexistence patch (`experimental/firmware-port/patch_camera44.py`,
SHA-256 `ec5633b17de329d5b18c3f8d8de6ebf389391a04193536d976ba167022d07b99`).

## Overlay onto a vendor tree

```sh
export ELEGOO_AI_CAMERA_SRC=/path/to/elegoo/extras/ai_camera
cd builder/current/components/ai_camera_timelapse
python3 apply_overlay.py "$ELEGOO_AI_CAMERA_SRC"
```

Then rebuild with the qualified ARM toolchain. **Do not** replace
`/opt/bin/ai_camera` over SSH until that rebuild yields a real ARM32
hard-float ELF and backup commands are written next to its SHA-256.

Intended printer install (once that ELF exists): **root SSH**, same class as
CC2 Control overlay — backup current `/opt/bin/ai_camera`, copy the new
binary, restart `ai_camera` only. Not unsigned USB firmware, not Dual Trust
OTA.

## Host test (parser and overlay copy)

```sh
gcc -DTL_CFG_TEST_MAIN -o /tmp/tl_cfg_test timelapse_cfg.c && /tmp/tl_cfg_test /path/to/timelapse.cfg
python3 -m unittest discover -s builder/current/tests -p 'test_timelapse_cfg_parser.py'
python3 -m unittest discover -s builder/current/tests -p 'test_apply_timelapse_overlay.py'
```

## Keys read from `[timelapse]`

`recording_mode`, `width`, `height`, `camera_fps`, `output_fps`, `bit_rate`, `crf`, `preset`, `tune`, `profile`, `level`, `pic_max_size`, `keep_source_jpegs`, `picture_dir`, `video_dir`, `temp_link`.

USB paths must start with `/mnt/exUDISK/` + subfolder (matches CC2 G-code USB mount).
