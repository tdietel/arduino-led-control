"""Arduino LED Control Module.

This module provides Python interface to communicate with Arduino devices
running the LED control firmware.
"""

__version__ = "0.1.0"
__author__ = "Your Name"
__email__ = "your.email@example.com"

from .controller import ArduinoController

__all__ = [
    "ArduinoController",
]
