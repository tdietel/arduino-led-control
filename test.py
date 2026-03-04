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

v, i = led.read_voltage_current()
print(f"Voltage: {v} V, Current: {i} mA")


# sleep(10e-3)
# print(led.read_response())
# sleep(1)
# print(led.read_response())
# led.led_off(pin=12)
# print(led.read_response())
# sleep(1)
# print(led.read_response())

# while True:
#     for w in [1, 4, 16, 32, 63]:
#         print(f"Generating single pulse with width {w} clock cycles...")
#         led.start_strobe(pulse_width_clk=w)
#         sleep(0.5)
