"""Firmware compilation and upload utilities."""

from __future__ import annotations

import json
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path
from typing import List

import argparse


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

    # print(f"Compiling and uploading {sketch_file} to {port} (FQBN: {fqbn})...")
    subprocess.run([
        "arduino-cli",
        "compile",
        "--upload",
        "-b", fqbn,
        "-p", port,
        str(sketch_file),
    ], text=True, check=True)


def check_arduino_cli() -> bool:
    """Check if arduino-cli is installed."""
    return shutil.which("arduino-cli") is not None


def main(argv: List[str] | None = None) -> int:
    """Main entry point for ledcontrol-firmware command."""
    parser = argparse.ArgumentParser(description="Compile and upload Arduino firmware")
    parser.add_argument("--sketch", default=None, help="Path to .ino sketch file")
    parser.add_argument("--fqbn", default="arduino:avr:uno", help="Board FQBN (default: arduino:avr:uno)")
    parser.add_argument("--port", default=None, help="Serial port (auto-detected if omitted)")
    args = parser.parse_args(argv)

    if not check_arduino_cli():
        print("arduino-cli not found. Install it first (e.g. brew install arduino-cli).", file=sys.stderr)
        return 1

    sketch = Path(args.sketch) if args.sketch else None
    if sketch is None:
        package_root = Path(__file__).resolve().parent
        sketch = package_root / "firmware" / "led_control.ino"

    if not sketch.exists():
        package_root = Path(__file__).resolve().parent
        alt_sketch = package_root / "firmware" / "firmware.ino"
        if alt_sketch.exists():
            sketch = alt_sketch
        else:
            print(f"Sketch not found: {sketch}", file=sys.stderr)
            return 1

    port = args.port
    if port is None:
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
