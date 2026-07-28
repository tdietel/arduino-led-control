"""Command-line frontend for arduino-led-control using cmd2.

Supports both REPL (shell) mode and single-command mode.
Run with no arguments for interactive shell, or with a command name and arguments for direct execution.
"""

from __future__ import annotations

import argparse
import sys
from typing import Optional

import cmd2

from .controller import ArduinoController


class LedControlShell(cmd2.Cmd):
    """Interactive shell for controlling Arduino LED devices."""

    def __init__(self, port: Optional[str] = None, baudrate: int = 115200, timeout: float = 2.0):
        """Initialize the shell with connection parameters."""
        super().__init__()
        self.port = port
        self.baudrate = baudrate
        self.timeout = timeout
        self.controller: Optional[ArduinoController] = None
        self.prompt = "(ledcontrol) "
        self.intro = "Arduino LED Controller. Type 'help' for available commands."

    def _get_controller(self) -> ArduinoController:
        """Get or create the controller instance."""
        if self.controller is None:
            try:
                self.controller = ArduinoController(
                    port=self.port, baudrate=self.baudrate, timeout=self.timeout
                )
            except Exception as exc:
                self.poutput(f"Failed to connect: {exc}", color="red")
                raise
        return self.controller

    def do_connect(self, arg: str) -> None:
        """Connect or reconnect to Arduino. Usage: connect [--port PORT] [--baud BAUD] [--timeout TIMEOUT]"""
        # Parse arguments
        parser = argparse.ArgumentParser(description="Connect to Arduino")
        parser.add_argument("--port", default=None, help="Serial port")
        parser.add_argument("--baud", type=int, default=None, help="Baud rate")
        parser.add_argument("--timeout", type=float, default=None, help="Timeout in seconds")
        parsed = parser.parse_args(arg.split() if arg else [])

        if parsed.port:
            self.port = parsed.port
        if parsed.baud:
            self.baudrate = parsed.baud
        if parsed.timeout:
            self.timeout = parsed.timeout

        # Force reconnection
        if self.controller:
            self.controller.close()
            self.controller = None

        try:
            self._get_controller()
            self.poutput(f"Connected to {self.port}", color="green")
        except Exception as exc:
            self.poutput(f"Connection failed: {exc}", color="red")

    def do_on(self, arg: str) -> None:
        """Turn LED on."""
        try:
            controller = self._get_controller()
            controller.on()
            self.poutput("LED turned ON", color="green")
        except Exception as exc:
            self.poutput(f"Failed to turn on LED: {exc}", color="red")

    def do_off(self, arg: str) -> None:
        """Turn LED off."""
        try:
            controller = self._get_controller()
            controller.off()
            self.poutput("LED turned OFF", color="green")
        except Exception as exc:
            self.poutput(f"Failed to turn off LED: {exc}", color="red")

    def do_dim(self, arg: str) -> None:
        """Set LED brightness. Usage: dim <level> (0-255)"""
        if not arg:
            self.poutput("Error: Please specify a brightness level (0-255)", color="red")
            return

        try:
            level = int(arg.strip())
        except ValueError:
            self.poutput("Error: Brightness level must be an integer", color="red")
            return

        if not (0 <= level <= 255):
            self.poutput("Error: Brightness level must be between 0 and 255", color="red")
            return

        try:
            controller = self._get_controller()
            controller.dim(level)
            self.poutput(f"LED brightness set to {level}", color="green")
        except Exception as exc:
            self.poutput(f"Failed to set brightness: {exc}", color="red")

    def do_strobe(self, arg: str) -> None:
        """Start strobe effect. Usage: strobe <frequency> [duration] [high] [low]"""
        if not arg:
            self.poutput("Error: Please specify a frequency", color="red")
            return

        parts = arg.split()
        try:
            frequency = float(parts[0]) if len(parts) > 0 else 1.0
            duration = int(parts[1]) if len(parts) > 1 else 200
            high = int(parts[2]) if len(parts) > 2 else 255
            low = int(parts[3]) if len(parts) > 3 else 0
        except (ValueError, IndexError):
            self.poutput("Error: Invalid arguments. Usage: strobe <frequency> [duration] [high] [low]",
                       color="red")
            return

        try:
            # print(f"Starting strobe: frequency={frequency} Hz, duration={duration} clk, high={high}, low={low}")
            controller = self._get_controller()
            controller.strobe(frequency, duration, high, low)
            duration_s = duration / 16000000
            if duration_s < 1e-3:
                duration_str = f"{duration_s * 1e6:.2f} µs"
            elif duration_s < 1:
                duration_str = f"{duration_s * 1e3:.2f} ms"
            else:
                duration_str = f"{duration_s:.2f} s"
            self.poutput(f"Strobe started: {frequency} Hz, duration = {duration} clk = {duration_str}, high={high}, low={low}",
                       color="green")
        except Exception as exc:
            self.poutput(f"Failed to start strobe: {exc}", color="red")

    def do_pcmtest(self, arg: str) -> None:
        """Start PCM pulse generation. Usage: pcm <frequency> [sample rate]
        
        Note: this test function sends a predefined PCM pulse."""
        if not arg:
            self.poutput("Error: Please specify a frequency", color="red")
            return

        parts = arg.split()
        try:
            frequency = float(parts[0]) if len(parts) > 0 else 1.0
            sample_period_clk = int(parts[1]) if len(parts) > 1 else 16
        except (ValueError, IndexError):
            self.poutput("Error: Invalid arguments. Usage: strobe <frequency> [sample rate]",
                       color="red")
            return

        pcmdata = bytes([255,255,0,255,255,255,0,0,0,255,255,0])

        try:
            controller = self._get_controller()
            controller.pcm(frequency, sample_period_clk, pcmdata)
            self.poutput(f"PCM started: {frequency} Hz, {len(pcmdata)} samples at {float(sample_period_clk) / 16.0}",
                       color="green")
        except Exception as exc:
            self.poutput(f"Failed to start PCM: {exc}", color="red")


    def do_status(self, arg: str) -> None:
        """Query Arduino status."""
        try:
            controller = self._get_controller()
            info = controller.get_status()
            for key, value in info.items():
                self.poutput(f"{key}: {value}")
        except Exception as exc:
            self.poutput(f"Failed to get status: {exc}", color="red")

    def do_quit(self, arg: str) -> bool:
        """Quit the shell."""
        if self.controller:
            self.controller.disconnect()
        self.poutput("Bye!")
        return True

    do_exit = do_quit
    do_q = do_quit


def main() -> int:
    """Main entry point."""
    # Parse command-line arguments
    # Connection options are optional, any positional args are the command to run
    parser = argparse.ArgumentParser(
        description="Arduino LED Controller - supports REPL and single-command modes"
    )
    parser.add_argument(
        "command",
        nargs="*",
        help="Command to execute (e.g., 'on', 'dim 128'). If omitted, starts REPL mode.",
    )
    parser.add_argument(
        "--port",
        default=None,
        help="Serial port (auto-detect if omitted)",
    )
    parser.add_argument(
        "--baud", "--baudrate",
        type=int,
        default=115200,
        help="Serial baud rate (default: 115200)",
    )
    parser.add_argument(
        "--timeout",
        type=float,
        default=2.0,
        help="Serial timeout in seconds (default: 2.0)",
    )

    args = parser.parse_args()

    # Create shell instance
    shell = LedControlShell(port=args.port, baudrate=args.baud, timeout=args.timeout)

    # Single-command mode: if command positional args are provided
    if args.command:
        cmd_name = args.command[0] if args.command else ""
        cmd_args = " ".join(args.command[1:]) if len(args.command) > 1 else ""

        try:
            # Execute the command
            shell.onecmd(f"{cmd_name} {cmd_args}")
            return 0
        except SystemExit:
            return 1
        except Exception as exc:
            print(f"Error: {exc}", file=sys.stderr)
            return 1

    # REPL mode
    try:
        shell.cmdloop()
        return 0
    except Exception as exc:
        print(f"Error: {exc}", file=sys.stderr)
        return 1

