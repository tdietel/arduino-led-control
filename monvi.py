#!/usr/bin/env python3

from arduino_led_control import ArduinoController
from time import sleep

led = ArduinoController()
print(f"Connected to Arduino on port {led.port}")

try:
    if led.read_response(timeout=2.0) != "READY":
        # print("Unexpected response from Arduino. Expected 'READY'.")
        raise ConnectionError("Arduino did not send READY signal.")
except TimeoutError:
    print("Arduino did not become ready in time.")

led.led_on(pin=12)
sleep(1)

while True:
    v, i = led.read_voltage_current()
    print(f"Voltage: {v} V, Current: {i} mA")
    sleep(0.5)
