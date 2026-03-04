#!/usr/bin/env python3

from arduino_led_control import ArduinoController
from time import sleep

led = ArduinoController()
print(f"Connected to Arduino on port {led.port}")

led.led_on(pin=12)
sleep(10e-3)
print(led.read_response())
sleep(1)
led.led_off(pin=12)