#!/usr/bin/env python3
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1] / "components" / "ai_camera_timelapse"
SCRIPT = ROOT / "apply_overlay.py"


class ApplyTimelapseOverlay(unittest.TestCase):
    def test_copies_parser_and_runtime_sources(self):
        with tempfile.TemporaryDirectory() as tmp:
            target = Path(tmp) / "ai_camera"
            (target / "module/camera").mkdir(parents=True)
            subprocess.check_call(["python3", str(SCRIPT), str(target)])
            expected = [
                "module/camera/timelapse_cfg.c",
                "module/camera/timelapse_cfg.h",
                "module/camera/timelapse_runtime.c",
                "module/camera/timelapse_runtime.h",
                "module/camera/timelapse_path.c",
                "module/camera/timelapse_path.h",
                "module/camera/timelapse_quality.h",
            ]
            for rel in expected:
                path = target / rel
                self.assertTrue(path.is_file(), rel)
                self.assertGreater(path.stat().st_size, 0, rel)


if __name__ == "__main__":
    unittest.main()
