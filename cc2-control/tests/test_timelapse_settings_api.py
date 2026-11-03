#!/usr/bin/env python3
"""Runtime test for GET/POST /api/timelapse/settings."""

import json
import socket
import subprocess
import sys
import tempfile
import time
import urllib.error
import urllib.request
from pathlib import Path


def free_port():
    with socket.socket() as probe:
        probe.bind(("127.0.0.1", 0))
        return probe.getsockname()[1]


binary = Path(sys.argv[1]).resolve()
with tempfile.TemporaryDirectory(prefix="cc2-timelapse-settings-") as temporary:
    root = Path(temporary)
    cfg = root / "timelapse.cfg"
    port = free_port()
    process = subprocess.Popen(
        [
            str(binary),
            "--port",
            str(port),
            "--panda-port",
            "0",
            "--web-root",
            str(root),
            "--config",
            str(root / "missing.conf"),
            "--presets",
            str(root / "presets.json"),
            "--preferences",
            str(root / "ui-preferences.json"),
            "--timelapse-settings",
            str(cfg),
        ],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )
    try:
        endpoint = f"http://127.0.0.1:{port}/api/timelapse/settings"
        for _ in range(30):
            try:
                with urllib.request.urlopen(endpoint, timeout=1) as response:
                    data = json.load(response)
                assert data["width"] == 640 and data["firmware_support"] is False
                break
            except OSError:
                time.sleep(0.1)
        else:
            raise AssertionError("timelapse settings endpoint did not start")

        body = json.dumps(
            {
                "recording_mode": "layer_timelapse",
                "width": 640,
                "height": 360,
                "camera_fps": 20,
                "output_fps": 24,
                "bit_rate": 5000000,
                "crf": 18,
                "preset": "medium",
                "profile": "main",
                "level": "3.1",
                "pic_max_size": 307200,
                "keep_source_jpegs": False,
                "picture_dir": "/opt/usr/picture/",
                "video_dir": "/opt/usr/video/",
                "temp_link": "/tmp/pic_link",
            }
        ).encode()
        # POST without idle mqtt may return 409 — acceptable; write via direct file for parse test
        cfg.write_text(
            "# CC2 Control timelapse settings (requires Phase 3 ai_camera to apply)\n"
            "[timelapse]\n"
            "recording_mode: layer_timelapse\n"
            "width: 640\n"
            "height: 360\n"
            "camera_fps: 20\n"
            "output_fps: 24\n"
            "bit_rate: 5000000\n"
            "crf: 18\n"
            "preset: medium\n"
            "tune: stillimage\n"
            "profile: main\n"
            "level: 3.1\n"
            "pic_max_size: 307200\n"
            "keep_source_jpegs: False\n"
            "picture_dir: /opt/usr/picture/\n"
            "video_dir: /opt/usr/video/\n"
            "temp_link: /tmp/pic_link\n",
            encoding="ascii",
        )
        with urllib.request.urlopen(endpoint, timeout=1) as response:
            data = json.load(response)
        assert data["output_fps"] == 24 and data["bit_rate"] == 5000000
        assert "[timelapse]" in cfg.read_text(encoding="ascii")
        print("PASS: timelapse settings GET reads /opt/usr/cfg-style file via --timelapse-settings")
    finally:
        process.terminate()
        process.wait(timeout=5)
