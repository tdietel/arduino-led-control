"""Command-line frontend for arduino-led-control."""

from __future__ import annotations

from functools import wraps

import click

from . import ArduinoController

def serial_options(f):
    """Decorator to add common serial port options with auto-detection and provide controller."""
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
        # Create controller and pass it as the 'controller' argument
        try:
            controller = ArduinoController(port=port, baudrate=baudrate, timeout=timeout)
        except Exception as exc:
            click.echo(f"Failed to create controller: {exc}", err=True)
            raise SystemExit(1)
        
        return f(controller=controller, *args, **kwargs)
    return wrapper


@click.group()
def cli() -> None:
    """Control Arduino LED devices."""
    pass


@cli.command()
@serial_options
def on(controller: ArduinoController) -> None:
    """Turn on LED."""
    try:
        controller.led_on()
        click.echo("LED turned ON")
    except Exception as exc:  # noqa: BLE001
        click.echo(f"Failed to turn on LED: {exc}", err=True)
        raise SystemExit(1)


@cli.command()
@serial_options
def off(controller: ArduinoController) -> None:
    """Turn off LED."""
    try:
        controller.led_off()
        click.echo("LED turned OFF")
    except Exception as exc:  # noqa: BLE001
        click.echo(f"Failed to turn off LED: {exc}", err=True)
        raise SystemExit(1)

@cli.command()
@serial_options
@click.argument("level", type=int)
def dim(controller: ArduinoController, level: int) -> None:
    """Set LED brightness (0-255)."""
    if not (0 <= level <= 255):
        click.echo("Brightness level must be between 0 and 255.", err=True)
        raise SystemExit(1)

    try:
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
def strobe(controller: ArduinoController, frequency: float, duration: int, high: int, low: int) -> None:
    """Start strobe effect with given frequency (Hz), duration (clk cycles), and high/low brightness levels."""
    try:
        controller.set_pulse(duration, high, low)
        controller.start_strobe(frequency)
        click.echo(f"Strobe started: {frequency} Hz, {duration} clk cycles duration, high={high}, low={low}")
    except Exception as exc:  # noqa: BLE001
        click.echo(f"Failed to start strobe: {exc}", err=True)
        raise SystemExit(1)

@cli.command()
@serial_options
def status(controller: ArduinoController) -> None:
    """Query Arduino strobe status."""
    try:
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
