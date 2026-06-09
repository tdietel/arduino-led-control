"""Firmware compilation and upload utilities."""

from __future__ import annotations

import json
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path
from typing import List


def run_cmd(cmd: List[str]) -> None:
    result = subprocess.run(cmd, text=True)
    if result.returncode != 0:
        raise SystemExit(result.returncode)


def auto_detect_port() -> str:
    """Auto-detect Arduino serial port."""
    result = subprocess.run(
        ["arduino-cli", "board", "list", "--json"],
        capture_output=True,
        text=True,
        check=False,
    )
    if result.returncode != 0:
        raise RuntimeError("Failed to run 'arduino-cli board list'")

    data = json.loads(result.stdout)
    detected = data.get("detected_ports", [])
    if not detected:
        raise RuntimeError("No serial devices found. Connect your Arduino and try again.")

    ch340_port = None # Detect CH340-based boards if no better match is found
    for entry in detected:
        if entry.get("matching_boards"):
            return entry["port"]["address"]
        
        props = entry.get("port", {}).get("properties", {})
        if props.get("vid") == "0x1A86" and props.get("pid") == "0x7523":
            ch340_port = entry["port"]["address"]

    if ch340_port:
        return ch340_port

    raise RuntimeError("No Arduino board found. Connect your Arduino and try again.")


def compile_and_upload(sketch_file: Path, fqbn: str, port: str) -> None:
    """Compile and upload firmware to Arduino."""
    if not sketch_file.exists():
        raise FileNotFoundError(f"Sketch not found: {sketch_file}")

    sketch_name = sketch_file.stem

    with tempfile.TemporaryDirectory(prefix="arduino_upload_") as tmp:
        tmp_root = Path(tmp)
        tmp_sketch_dir = tmp_root / sketch_name
        tmp_sketch_dir.mkdir(parents=True, exist_ok=True)

        tmp_sketch_file = tmp_sketch_dir / f"{sketch_name}.ino"
        shutil.copy2(sketch_file, tmp_sketch_file)

        cmd = [
            "arduino-cli",
            "compile",
            "--upload",
            "-b",
            fqbn,
            "-p",
            port,
            str(tmp_sketch_dir),
        ]
        run_cmd(cmd)


def check_arduino_cli() -> bool:
    """Check if arduino-cli is installed."""
    return shutil.which("arduino-cli") is not None
