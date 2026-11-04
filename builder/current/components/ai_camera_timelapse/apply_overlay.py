#!/usr/bin/env python3
"""Apply Murin timelapse overlay onto an ELEGOO ai_camera source tree.

Usage:
  python3 apply_overlay.py /path/to/elegoo/extras/ai_camera

Requires a full vendor checkout (not shipped in this repo). Copies overlay
headers and C files plus the shared parser. Encoder hunks still live in
experimental/timelapse-phase3-ai_camera-sketch.patch and must be applied
manually on the vendor tree. This script does not produce /opt/bin/ai_camera.
"""
import json
import shutil
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent
OVERLAY = ROOT / "overlay"


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: apply_overlay.py AI_CAMERA_SRC_ROOT", file=sys.stderr)
        return 2
    target = Path(sys.argv[1]).resolve()
    if not target.is_dir():
        print("target is not a directory", file=sys.stderr)
        return 1
    manifest = json.loads((OVERLAY / "MANIFEST.json").read_text(encoding="utf-8"))
    for rel in manifest.get("copy_headers", []):
        src = OVERLAY / rel
        dst = target / rel
        dst.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(src, dst)
        print("copied", rel)
    for rel in manifest.get("copy_sources", []):
        src = OVERLAY / rel
        dst = target / rel
        dst.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(src, dst)
        print("copied", rel)
    for name in ("timelapse_cfg.c", "timelapse_cfg.h"):
        src = ROOT / name
        dst = target / "module/camera" / name
        dst.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(src, dst)
        print("copied module/camera/" + name)
    print("Next: apply experimental/timelapse-phase3-ai_camera-sketch.patch, add the copied .c files to the vendor CMake target, then rebuild with the printer GCC 6.5.0/glibc 2.23 toolchain. See BUILD.md. Do not replace /opt/bin/ai_camera until that ARM ELF exists.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
