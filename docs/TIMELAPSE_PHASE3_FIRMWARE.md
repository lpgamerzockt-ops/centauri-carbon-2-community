# Timelapse Phase 3 — firmware (`ai_camera`) integration sketch

CC2 Control already stores operator settings in `/opt/usr/cfg/timelapse.cfg` (`GET`/`POST /api/timelapse/settings`). **Stock `ai_camera` does not read this file.** Changing MP4 quality (bitrate, CRF, x264 preset) or USB storage paths at runtime requires a **rebuilt `ai_camera` component** bundled in Community Firmware — not a CC2 Control `.cc2update` or SSH application overlay alone.

## Status (optimierung)

| Piece | In Git | In a flashable image |
| --- | --- | --- |
| Parser `timelapse_cfg.c` / overlay sources | yes — `builder/current/components/ai_camera_timelapse/` | **no** |
| `apply_overlay.py` | yes | copies sources only; needs a vendor checkout |
| Vendor encoder hunks | sketch patch only | **no** rebuilt `/opt/bin/ai_camera` |
| Firmware builder camera pin | still stock coexistence SHA `ec5633b1…d07b99` | overlay is **not** consumed |

This workspace does **not** contain `keys/cc2_community_release_private.pem`,
`keys/cc2_aes_key_v1.bin`, or `original_firmware/cc2_eeb001_02.01.01.07_20260922210235.zip.sig`.
The builder refuses those gaps. Placeholder signatures are not created.

## What a `.cc2update` installs

- CC2 Control web UI and backend on the printer.
- Does **not** replace vendor `ai_camera` unless the release explicitly ships a new camera/firmware bundle.

## Encoder Stufe A (640×360, configurable quality)

Target behaviour (unchanged default resolution unless cfg sets otherwise):

| Setting | CC2 cfg key | Firmware hook (proposal) |
| --- | --- | --- |
| Bitrate | `bit_rate` | `video_encoder.cpp` |
| CRF | `crf` | `video_encoder.cpp` |
| x264 preset | `preset` | `video_encoder.cpp` |
| Paths | `picture_dir`, `video_dir`, `temp_link` | `time_lapse_photo.h` / path validator |

Reference patch (ELEGOO tree): `experimental/timelapse-phase3-ai_camera-sketch.patch`. **Applied integration in this repo:** `builder/current/components/ai_camera_timelapse/` (`timelapse_cfg.c`, overlay sources, `BUILD.md`, `apply_overlay.py`).

Integration path for maintainers:

1. Copy overlay onto vendor `ai_camera` with `apply_overlay.py` (see `BUILD.md`).
2. Link `timelapse_cfg.c`, `timelapse_runtime.c`, `timelapse_path.c`; call `tl_runtime_load("/opt/usr/cfg/timelapse.cfg")` at startup.
3. Apply remaining encoder hunks; rebuild `/opt/bin/ai_camera`.
4. Ship in a **full Community Firmware** release with Dual Trust signatures — not via `.cc2update` alone. Update `CAMERA_RELEASE_SHA256` only to the measured hash of that binary.

## USB storage paths

CC2 validates `picture_dir` / `video_dir` under `/opt/usr/…` or `/mnt/exUDISK/<subfolder>/` (same USB mount as G-code: default `/mnt/exUDISK`, `main.c` `GCODE_USB_ROOT`). Phase 3 firmware must open those paths; CC2 only persists and validates them.

## USB vs SSH (user install only)

| Method | File | Who |
| --- | --- | --- |
| USB offline firmware | signed `.zip.sig` on USB root → Settings → Check for Updates → Offline Update | printer Dual Trust |
| SSH | CC2 Control Multiplatform ZIP (`install-cc2-control.sh`) | application only — **does not** replace `ai_camera` |
| Unsigned RootFS / guessed `.zip.sig` | not installable | Dual Trust refuses |

Agents do not flash the printer.

## Manuel / Murin fork

Murin `optimierung` / PR #1 SSH overlay: UI presets (Low/Medium/High), disk-space hints, USB allowlist, German UI. **MP4 quality and USB frame/MP4 directories take effect only after Phase 3 firmware** that reads `timelapse.cfg`.
