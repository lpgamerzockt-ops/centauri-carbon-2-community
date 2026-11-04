# Timelapse Phase 3 — firmware (`ai_camera`) integration sketch

CC2 Control already stores operator settings in `/opt/usr/cfg/timelapse.cfg` (`GET`/`POST /api/timelapse/settings`). **Stock `ai_camera` does not read this file.** Changing MP4 quality (bitrate, CRF, x264 preset) at runtime requires a **rebuilt `ai_camera` component** bundled in Community Firmware — not a CC2 Control `.cc2update` alone.

## What a `.cc2update` installs

- CC2 Control web UI and backend on the printer.
- Does **not** replace vendor `ai_camera` unless the release explicitly ships a new camera/firmware bundle.

## Encoder Stufe A (640×360, configurable quality)

Target behaviour (unchanged resolution):

| Setting | CC2 cfg key | Firmware hook (proposal) |
| --- | --- | --- |
| Bitrate | `bit_rate` | `video_encoder.cpp` |
| CRF | `crf` | `video_encoder.cpp` |
| x264 preset | `preset` | `video_encoder.cpp` |
| Paths | `picture_dir`, `video_dir`, `temp_link` | `time_lapse_photo.h` / path validator |

Reference patch (ELEGOO tree): project store `timelapse-vorschlag.patch`. **Applied integration in this repo:** `builder/current/components/ai_camera_timelapse/` (`timelapse_cfg.c`, overlay sources, `BUILD.md`, `apply_overlay.py`).

Integration path for maintainers:

1. Copy overlay onto vendor `ai_camera` with `apply_overlay.py` (see `BUILD.md`).
2. Link `timelapse_cfg.c`, `timelapse_runtime.c`, `timelapse_path.c`; call `tl_runtime_load("/opt/usr/cfg/timelapse.cfg")` at startup.
3. Rebuild `/opt/bin/ai_camera` with the printer GCC 6.5.0/glibc 2.23 toolchain. This repo does **not** ship that toolchain or the vendor tree; overlay sources alone cannot emit an ELF.
4. Once a real ARM32 hard-float ELF exists (with SHA-256 and backup commands), install it like CC2 Control: **root SSH** replace of `/opt/bin/ai_camera`, then restart the camera process. Do **not** use unsigned USB firmware. A full Dual-Trust OTA is optional, not required for a single-binary overlay.

## USB storage paths

CC2 validates `picture_dir` / `video_dir` under `/opt/usr/…` or `/mnt/exUDISK/<subfolder>/` (same USB mount as G-code: default `/mnt/exUDISK`, `main.c` `GCODE_USB_ROOT`). Phase 3 firmware must open those paths; CC2 only persists and validates them.

## Manuel / Murin fork

Murin `optimierung` branch: UI presets (Low/Medium/High), disk-space hints, USB allowlist, German UI. **MP4 quality changes take effect only after a rebuilt `ai_camera` that reads `timelapse.cfg`.** Until that ARM ELF exists, do not replace the printer binary.
