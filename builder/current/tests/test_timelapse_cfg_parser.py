#!/usr/bin/env python3
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1] / "components" / "ai_camera_timelapse"
CFG_SAMPLE = (
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
    "temp_link: /tmp/pic_link\n"
)


def compile_parser(tmp: Path) -> Path:
    out = tmp / "tl_test"
    subprocess.check_call(
        ["gcc", "-DTL_CFG_TEST_MAIN", "-o", str(out), str(ROOT / "timelapse_cfg.c")]
    )
    return out


class TimelapseCfgParser(unittest.TestCase):
    def test_usb_paths(self):
        with tempfile.TemporaryDirectory() as tmp:
            tmp_path = Path(tmp)
            f = tmp_path / "timelapse.cfg"
            f.write_text(CFG_SAMPLE, encoding="ascii")
            result = subprocess.check_output([str(compile_parser(tmp_path)), str(f)], text=True)
            self.assertIn("OK width=854", result)
            self.assertIn("/mnt/exUDISK/timelapse/picture/", result)

    def test_rejects_usb_root_without_subfolder(self):
        with tempfile.TemporaryDirectory() as tmp:
            tmp_path = Path(tmp)
            f = tmp_path / "timelapse.cfg"
            f.write_text(
                CFG_SAMPLE.replace(
                    "picture_dir: /mnt/exUDISK/timelapse/picture/",
                    "picture_dir: /mnt/exUDISK/",
                ),
                encoding="ascii",
            )
            proc = subprocess.run(
                [str(compile_parser(tmp_path)), str(f)],
                capture_output=True,
                text=True,
            )
            self.assertNotEqual(proc.returncode, 0)
            self.assertIn("Invalid storage path", proc.stderr)

    def test_apply_overlay_copies_runtime_sources(self):
        with tempfile.TemporaryDirectory() as tmp:
            target = Path(tmp) / "ai_camera"
            target.mkdir()
            proc = subprocess.run(
                ["python3", str(ROOT / "apply_overlay.py"), str(target)],
                check=True,
                capture_output=True,
                text=True,
            )
            self.assertIn("timelapse-phase3-ai_camera-sketch.patch", proc.stdout)
            for rel in (
                "module/camera/timelapse_cfg.c",
                "module/camera/timelapse_cfg.h",
                "module/camera/timelapse_runtime.c",
                "module/camera/timelapse_runtime.h",
                "module/camera/timelapse_path.c",
                "module/camera/timelapse_path.h",
                "module/camera/timelapse_quality.h",
            ):
                self.assertTrue((target / rel).is_file(), rel)


if __name__ == "__main__":
    unittest.main()
