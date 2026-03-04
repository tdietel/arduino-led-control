"""Command-line frontend for arduino-led-control."""

from __future__ import annotations

from functools import wraps
from pathlib import Path

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
def firmware_upload(sketch: Path | None, fqbn: str, port: str | None) -> None:
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
@click.option(
    "--pin",
    type=int,
    default=13,
    help="Arduino pin number (default: 13)",
)
@serial_options
def on(port: str, baudrate: int, timeout: float, pin: int) -> None:
    """Turn on LED."""
    try:
        # click.echo(f"Connecting to Arduino on port {port}...")
        controller = ArduinoController(port=port, baudrate=baudrate, timeout=timeout)
        controller.led_on(pin)
        # controller.close()
        click.echo(f"LED on pin {pin} turned ON")
    except Exception as exc:  # noqa: BLE001
        click.echo(f"Failed to turn on LED: {exc}", err=True)
        raise SystemExit(1)


@cli.command()
@click.option(
    "--pin",
    type=int,
    default=13,
    help="Arduino pin number (default: 13)",
)
@serial_options
def off(port: str, baudrate: int, timeout: float, pin: int) -> None:
    """Turn off LED."""
    try:
        controller = ArduinoController(port=port, baudrate=baudrate, timeout=timeout)
        controller.led_off(pin)
        # controller.close()
        click.echo(f"LED on pin {pin} turned OFF")
    except Exception as exc:  # noqa: BLE001
        click.echo(f"Failed to turn off LED: {exc}", err=True)
        raise SystemExit(1)


def main() -> int:
    """Main entry point."""
    try:
        cli()
        return 0
    except SystemExit as e:
        return e.code or 0
