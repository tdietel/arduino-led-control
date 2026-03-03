#!/usr/bin/env python3
"""Compile and upload Arduino firmware using arduino-cli.

This script works around the Arduino sketch naming rule by creating a temporary
sketch folder named after the .ino file before invoking arduino-cli.
"""

from __future__ import annotations

import argparse
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path


def run_cmd(cmd: list[str]) -> None:
    result = subprocess.run(cmd, text=True)
    if result.returncode != 0:
        raise SystemExit(result.returncode)


def auto_detect_port() -> str:
    result = subprocess.run(
        ["arduino-cli", "board", "list"],
        capture_output=True,
        text=True,
        check=False,
    )
    if result.returncode != 0:
        raise RuntimeError("Failed to run 'arduino-cli board list'")

    lines = [ln for ln in result.stdout.splitlines() if ln.strip()]
    if len(lines) <= 1:
        raise RuntimeError("No serial devices found. Connect your Arduino and try again.")

    # Prefer USB serial entries on macOS/Linux.
    for line in lines[1:]:
        port = line.split()[0]
        if "usb" in port.lower() or "ttyACM" in port or "ttyUSB" in port:
            return port

    # Fallback: first listed port.
    return lines[1].split()[0]


def compile_and_upload(sketch_file: Path, fqbn: str, port: str) -> None:
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


def main() -> int:
    repo_root = Path(__file__).resolve().parent.parent
    default_sketch = repo_root / "arduino_led_control" / "firmware" / "led_control.ino"

    parser = argparse.ArgumentParser(description="Compile and upload Arduino firmware")
    parser.add_argument(
        "--sketch",
        type=Path,
        default=default_sketch,
        help=f"Path to .ino sketch (default: {default_sketch})",
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

    args = parser.parse_args()

    if shutil.which("arduino-cli") is None:
        print("arduino-cli not found. Install it first (e.g. brew install arduino-cli).", file=sys.stderr)
        return 1

    port = args.port
    if not port:
        try:
            port = auto_detect_port()
            print(f"Auto-detected port: {port}")
        except Exception as exc:  # noqa: BLE001
            print(f"Could not auto-detect port: {exc}", file=sys.stderr)
            return 2

    try:
        compile_and_upload(args.sketch, args.fqbn, port)
    except Exception as exc:  # noqa: BLE001
        print(f"Upload failed: {exc}", file=sys.stderr)
        return 3

    print("Firmware upload complete.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
