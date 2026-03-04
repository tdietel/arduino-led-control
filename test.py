#!/usr/bin/env python3

from arduino_led_control import ArduinoController
from time import sleep

led = ArduinoController()
print(f"Connected to Arduino on port {led.port}")

while led.read_response() != "READY":
    print("Waiting for Arduino to be ready...")
    sleep(0.2)

led.led_on(pin=12)
sleep(10e-3)
print(led.read_response())
sleep(1)
print(led.read_response())
led.led_off(pin=12)
print(led.read_response())
sleep(1)
print(led.read_response())
