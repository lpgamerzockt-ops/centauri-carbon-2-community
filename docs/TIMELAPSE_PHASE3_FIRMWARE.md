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

Proposal patch (not applied in this repo’s vendor tree): see project store `timelapse-vorschlag.patch` and analysis in `timelapse-analyse.md` under the agent store `docs/`.

Integration path for maintainers:

1. Add `[timelapse]` parser in firmware build (builder/vendor `ai_camera` sources).
2. Load cfg at `ai_camera` startup; fall back to compile-time defaults when missing.
3. Ship in a **full Community Firmware** release (e.g. V4.4.x+) with signed artifacts.

## USB storage paths

CC2 validates `picture_dir` / `video_dir` under `/opt/usr/…` or `/mnt/exUDISK/<subfolder>/` (same USB mount as G-code: default `/mnt/exUDISK`, `main.c` `GCODE_USB_ROOT`). Phase 3 firmware must open those paths; CC2 only persists and validates them.

## Manuel / Murin fork

Murin `optimierung` branch: UI presets (Low/Medium/High), disk-space hints, USB allowlist, German UI. **MP4 quality changes take effect only after Phase 3 firmware** that reads `timelapse.cfg`.
