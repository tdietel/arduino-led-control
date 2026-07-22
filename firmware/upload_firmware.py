#!/usr/bin/env python3
"""Compile and upload Arduino firmware using arduino-cli.

This script optionally works around the Arduino sketch naming rule by creating a temporary
sketch folder named after the .ino file before invoking arduino-cli, but only
when the sketch file's stem doesn't match its parent directory name.
"""

from __future__ import annotations

import argparse
import json
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path
from typing import List


def run_cmd(cmd: List[str]) -> None:
    """Run a command, exit on failure."""
    result = subprocess.run(cmd, text=True)
    if result.returncode != 0:
        raise SystemExit(result.returncode)


def check_arduino_cli() -> bool:
    """Check if arduino-cli is installed."""
    return shutil.which("arduino-cli") is not None


def auto_detect_port() -> str:
    """Auto-detect Arduino serial port using arduino-cli with JSON output.
    
    Prefers boards with matching_boards, falls back to CH340-based boards
    (VID:PID 0x1A86:0x7523).
    """
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

    # First pass: prefer boards with matching_boards
    for entry in detected:
        if entry.get("matching_boards"):
            return entry["port"]["address"]

    # Second pass: detect CH340-based boards
    ch340_port = None
    for entry in detected:
        props = entry.get("port", {}).get("properties", {})
        if props.get("vid") == "0x1A86" and props.get("pid") == "0x7523":
            ch340_port = entry["port"]["address"]

    if ch340_port:
        return ch340_port

    raise RuntimeError("No Arduino board found. Connect your Arduino and try again.")

def compile_and_upload(sketch_file: Path, fqbn: str, port: str) -> None:
    if not sketch_file.exists():
        raise FileNotFoundError(f"Sketch not found: {sketch_file}")

    sketch_name = sketch_file.stem
    sketch_dir = sketch_file.parent

    basecmd = [ "arduino-cli", "compile", "--upload", "-b", fqbn, "-p", port ]

    # Arduino requires the .ino file to be in a directory with the same name
    if sketch_dir.name == sketch_name:
        # Directory name matches sketch stem - use directly
        result = subprocess.run(basecmd + [str(sketch_file)], text=True)
        if result.returncode != 0:
            raise SystemExit(result.returncode)

    else:
        # Directory name doesn't match - use temp dir workaround
        with tempfile.TemporaryDirectory(prefix="arduino_upload_") as tmp:
            tmp_root = Path(tmp)
            tmp_sketch_dir = tmp_root / sketch_name
            tmp_sketch_dir.mkdir(parents=True, exist_ok=True)

            tmp_sketch_file = tmp_sketch_dir / f"{sketch_name}.ino"
            shutil.copy2(sketch_file, tmp_sketch_file)

        result = subprocess.run(basecmd + [str(tmp_sketch_file)], text=True)
        if result.returncode != 0:
            raise SystemExit(result.returncode)

def main(argv: List[str] | None = None) -> int:
    """Main entry point for firmware upload."""
    repo_root = Path(__file__).resolve().parent.parent
    default_sketch = repo_root / "firmware" / "led_control" / "led_control.ino"

    parser = argparse.ArgumentParser(description="Compile and upload Arduino firmware")
    parser.add_argument(
        "--sketch",
        type=Path,
        default=default_sketch,
        help=f"Path to .ino sketch file (default: {default_sketch})",
    )
    parser.add_argument(
        "--fqbn",
        default="arduino:avr:uno",
        help="Board FQBN (default: arduino:avr:uno)",
    )
    parser.add_argument(
        "--port",
        default=None,
        help="Serial port (auto-detected if omitted)",
    )

    args = parser.parse_args(argv)

    if not check_arduino_cli():
        print("arduino-cli not found. Install it first (e.g. brew install arduino-cli).", file=sys.stderr)
        return 1

    sketch = args.sketch
    if not sketch.exists():
        # Fallback: try old location
        old_sketch = repo_root / "arduino_led_control" / "firmware" / "led_control.ino"
        if old_sketch.exists():
            sketch = old_sketch
        else:
            print(f"Sketch not found: {sketch}", file=sys.stderr)
            return 1

    port = args.port
    if not port:
        try:
            port = auto_detect_port()
            print(f"Auto-detected port: {port}")
        except Exception as exc:
            print(f"Could not auto-detect port: {exc}", file=sys.stderr)
            return 2

    try:
        print(f"Using sketch: {sketch}")
        compile_and_upload(sketch, args.fqbn, port)
        print("Firmware upload complete.")
        return 0
    except Exception as exc:
        print(f"Upload failed: {exc}", file=sys.stderr)
        return 3


if __name__ == "__main__":
    raise SystemExit(main())
