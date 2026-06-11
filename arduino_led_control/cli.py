"""Command-line frontend for arduino-led-control."""

from __future__ import annotations

from functools import wraps
from pathlib import Path
from typing import Optional

import click

from . import ArduinoController
from .firmware import auto_detect_port, check_arduino_cli, compile_and_upload


def serial_options(f):
    """Decorator to add common serial port options with auto-detection."""
    @click.option(
        "--port",
        default=None,
        help="Serial port (auto-detect if omitted)",
    )
    @click.option(
        "--baudrate",
        type=int,
        default=115200,
        help="Serial baud rate (default: 115200)",
    )
    @click.option(
        "--timeout",
        type=float,
        default=1.0,
        help="Serial timeout in seconds (default: 1.0)",
    )
    @wraps(f)
    def wrapper(port, baudrate, timeout, *args, **kwargs):
        # Auto-detect port if not specified
        if port is None:
            try:
                port = auto_detect_port()
                click.echo(f"Auto-detected port: {port}")
            except Exception as exc:  # noqa: BLE001
                click.echo(f"Could not auto-detect port: {exc}", err=True)
                raise SystemExit(1)
        return f(port=port, baudrate=baudrate, timeout=timeout, *args, **kwargs)
    return wrapper


@click.group()
def cli() -> None:
    """Control Arduino LED devices."""
    pass


@cli.command()
@click.option(
    "--sketch",
    type=click.Path(exists=True, path_type=Path),
    default=None,
    help="Path to .ino sketch",
)
@click.option(
    "--fqbn",
    default="arduino:avr:uno",
    help="Board FQBN (default: arduino:avr:uno)",
)
@click.option(
    "--port",
    default=None,
    help="Serial port (auto-detected if omitted)",
)
def flash(sketch: Optional[Path], fqbn: str, port: Optional[str]) -> None:
    """Compile and upload Arduino firmware."""
    if not check_arduino_cli():
        click.echo("arduino-cli not found. Install it first (e.g. brew install arduino-cli).", err=True)
        raise SystemExit(1)

    if sketch is None:
        package_root = Path(__file__).resolve().parent
        sketch = package_root / "firmware" / "led_control.ino"

    if not sketch.exists():
        click.echo(f"Sketch not found: {sketch}", err=True)
        raise SystemExit(1)

    detected_port = port
    if not detected_port:
        try:
            detected_port = auto_detect_port()
            click.echo(f"Auto-detected port: {detected_port}")
        except Exception as exc:  # noqa: BLE001
            click.echo(f"Could not auto-detect port: {exc}", err=True)
            raise SystemExit(2)

    try:
        compile_and_upload(sketch, fqbn, detected_port)
        click.echo("Firmware upload complete.")
    except Exception as exc:  # noqa: BLE001
        click.echo(f"Upload failed: {exc}", err=True)
        raise SystemExit(3)


@cli.command()
@serial_options
def on(port: str, baudrate: int, timeout: float) -> None:
    """Turn on LED."""
    try:
        # click.echo(f"Connecting to Arduino on port {port}...")
        controller = ArduinoController(port=port, baudrate=baudrate, timeout=timeout)
        controller.led_on()
        # controller.close()
        click.echo("LED turned ON")
    except Exception as exc:  # noqa: BLE001
        click.echo(f"Failed to turn on LED: {exc}", err=True)
        raise SystemExit(1)


@cli.command()
@serial_options
def off(port: str, baudrate: int, timeout: float) -> None:
    """Turn off LED."""
    try:
        controller = ArduinoController(port=port, baudrate=baudrate, timeout=timeout)
        controller.led_off()
        # controller.close()
        click.echo("LED turned OFF")
    except Exception as exc:  # noqa: BLE001
        click.echo(f"Failed to turn off LED: {exc}", err=True)
        raise SystemExit(1)

@cli.command()
@serial_options
@click.argument("level", type=int)
def dim(port: str, baudrate: int, timeout: float, level: int) -> None:
    """Set LED brightness (0-255)."""
    if not (0 <= level <= 255):
        click.echo("Brightness level must be between 0 and 255.", err=True)
        raise SystemExit(1)

    try:
        controller = ArduinoController(port=port, baudrate=baudrate, timeout=timeout)
        controller.dim(level)
        click.echo(f"LED brightness set to {level}")
    except Exception as exc:  # noqa: BLE001
        click.echo(f"Failed to set brightness: {exc}", err=True)
        raise SystemExit(1)

@cli.command()
@serial_options
@click.argument("frequency", type=float, default=1.0)
@click.argument("duration", type=int, default=200)
@click.argument("high", type=int, default=255)
@click.argument("low", type=int, default=0)
def strobe(port: str, baudrate: int, timeout: float, frequency: float, duration: int, high: int, low: int) -> None:
    """Start strobe effect with given frequency (Hz), duration (ms), and high/low brightness levels."""
    try:
        controller = ArduinoController(port=port, baudrate=baudrate, timeout=timeout)
        controller.set_pulse(duration, high, low)
        controller.start_strobe(frequency)
        click.echo(f"Strobe started: {frequency} Hz, {duration} ms duration, high={high}, low={low}")
    except Exception as exc:  # noqa: BLE001
        click.echo(f"Failed to start strobe: {exc}", err=True)
        raise SystemExit(1)

@cli.command()
@serial_options
def status(port: str, baudrate: int, timeout: float) -> None:
    """Query Arduino strobe status."""
    try:
        controller = ArduinoController(port=port, baudrate=baudrate, timeout=timeout)
        info = controller.get_status()
        for key, value in info.items():
            click.echo(f"{key}: {value}")
    except Exception as exc:  # noqa: BLE001
        click.echo(f"Failed to get status: {exc}", err=True)
        raise SystemExit(1)


def main() -> int:
    """Main entry point."""
    try:
        cli()
        return 0
    except SystemExit as e:
        return e.code or 0
