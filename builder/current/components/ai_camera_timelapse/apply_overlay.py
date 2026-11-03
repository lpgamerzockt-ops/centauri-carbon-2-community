#!/usr/bin/env python3
"""Apply Murin timelapse overlay onto an ELEGOO ai_camera source tree.

Usage:
  python3 apply_overlay.py /path/to/elegoo/extras/ai_camera

Requires a full vendor checkout (not shipped in this repo). Copies overlay headers and
patches listed files using the unified diff in overlay/MANIFEST.json.
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
    cfg_src = ROOT / "timelapse_cfg.c"
    cfg_dst = target / "module/camera/timelapse_cfg.c"
    cfg_dst.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(cfg_src, cfg_dst)
    print("copied module/camera/timelapse_cfg.c")
    print("Next: rebuild ai_camera with your vendor toolchain; see BUILD.md")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
