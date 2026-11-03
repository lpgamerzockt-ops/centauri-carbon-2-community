#!/usr/bin/env python3
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1] / "components" / "ai_camera_timelapse"


class TimelapseCfgParser(unittest.TestCase):
    def test_usb_paths(self):
        cfg = ROOT / "timelapse_cfg.c"
        with tempfile.TemporaryDirectory() as tmp:
            f = Path(tmp) / "timelapse.cfg"
            f.write_text(
                "[timelapse]\n"
                "recording_mode: layer_timelapse\n"
                "width: 854\n"
                "height: 480\n"
                "camera_fps: 20\n"
                "output_fps: 30\n"
                "bit_rate: 4000000\n"
                "crf: 20\n"
                "preset: medium\n"
                "tune: stillimage\n"
                "profile: main\n"
                "level: 3.1\n"
                "pic_max_size: 409600\n"
                "keep_source_jpegs: False\n"
                "picture_dir: /mnt/exUDISK/timelapse/picture/\n"
                "video_dir: /mnt/exUDISK/timelapse/video/\n"
                "temp_link: /tmp/pic_link\n",
                encoding="ascii",
            )
            out = Path(tmp) / "tl_test"
            subprocess.check_call(["gcc", "-DTL_CFG_TEST_MAIN", "-o", str(out), str(cfg)])
            result = subprocess.check_output([str(out), str(f)], text=True)
            self.assertIn("OK width=854", result)
            self.assertIn("/mnt/exUDISK/timelapse/picture/", result)


if __name__ == "__main__":
    unittest.main()
