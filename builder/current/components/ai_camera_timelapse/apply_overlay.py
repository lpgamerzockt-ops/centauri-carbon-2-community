#!/usr/bin/env python3
"""Copy Murin Phase 3 sources onto an ELEGOO ai_camera checkout.

Usage:
  python3 apply_overlay.py /path/to/elegoo/extras/ai_camera

This repository does not ship vendor ai_camera sources or a rebuilt
/opt/bin/ai_camera. The firmware builder still pins the qualified coexistence
camera binary (CAMERA_RELEASE_SHA256) and does not consume this overlay.

After copying, apply experimental/timelapse-phase3-ai_camera-sketch.patch onto
the vendor tree (from the ELEGOO repo root, typically patch -p1) and rebuild
with the vendor ARM toolchain. Do not fake signatures.
"""
import json
import shutil
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent
OVERLAY = ROOT / "overlay"
REPO = ROOT.parents[3]


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: apply_overlay.py AI_CAMERA_SRC_ROOT", file=sys.stderr)
        return 2
    target = Path(sys.argv[1]).resolve()
    if not target.is_dir():
        print("target is not a directory", file=sys.stderr)
        return 1
    manifest = json.loads((OVERLAY / "MANIFEST.json").read_text(encoding="utf-8"))
    for rel in manifest.get("copy_overlay", []):
        src = OVERLAY / rel
        dst = target / rel
        dst.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(src, dst)
        print("copied", rel)
    for src_rel, dst_rel in manifest.get("copy_parser", []):
        src = ROOT / src_rel
        dst = target / dst_rel
        dst.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(src, dst)
        print("copied", dst_rel)
    hunks = REPO / manifest.get("vendor_hunks", "")
    print("parser and runtime helpers copied; encoder/path hunks are not applied here.")
    if hunks.is_file():
        print("vendor hunks (manual):", hunks)
        print("from the ELEGOO source root: patch -p1 <", hunks)
    print("Next: add the new .c files to the vendor target, call tl_runtime_load(")
    print('  "/opt/usr/cfg/timelapse.cfg") at startup, rebuild /opt/bin/ai_camera,')
    print("then ship it in a signed Community Firmware image. See BUILD.md.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
