"""Arduino LED Control Module.

This module provides Python interface to communicate with Arduino devices
running the LED control firmware.
"""

from importlib.metadata import PackageNotFoundError, version

try:
    __version__ = version("arduino-led-control")
except PackageNotFoundError:
    # Source tree import before installation.
    __version__ = "0+unknown"
__author__ = "Thomas Dietel"
__email__ = "tom@dietel.net"

from .controller import ArduinoController

__all__ = [
    "ArduinoController",
]
