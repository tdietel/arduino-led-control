"""Arduino LED Control Module.

This module provides Python interface to communicate with Arduino devices
running the LED control firmware.
"""

__version__ = "0.1.0"
__author__ = "Thomas Dietel"
__email__ = "tom@dietel.net"

from .controller import ArduinoController

__all__ = [
    "ArduinoController",
]
