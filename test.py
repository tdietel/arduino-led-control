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

# v, i = led.read_voltage_current()
# print(f"Voltage: {v} V, Current: {i} mA")

# led.led_off(pin=12)
# sleep(1)

# led.start_strobe(pulse_width_clk=63)

# sleep(10e-3)
# print(led.read_response())
# sleep(1)
# print(led.read_response())
# led.led_off(pin=12)
# print(led.read_response())
# sleep(1)
# print(led.read_response())

while True:
    cmd = input("Command (o/on=on, f/off=off, <num>=pulse, d/double=double pulse q=quit): ").strip().lower()
    if cmd in ("q", "quit", "exit"):
        break
    elif cmd in ("o", "on"):
        led.led_on(pin=12)
    elif cmd in ("f", "off"):
        led.led_off(pin=12)
    elif cmd in ("d", "double"):
        led.start_double_strobe(pulse_width_clk=80, pulse_gap_clk=16, pulse_width2_clk=96)
    elif cmd in ("-"):
        led.set_strobe_frequency(100)
    elif cmd in ("+"):
        led.set_strobe_frequency(10000)
    elif cmd.isdigit():
        width = int(cmd)
        print(f"Generating single pulse with width {width} clock cycles...")
        led.start_strobe(pulse_width_clk=width)
    else:
        print("Unknown command.")

    # for w in [1, 4, 16, 32, 63]:
    #     print(f"Generating single pulse with width {w} clock cycles...")
    #     led.start_strobe(pulse_width_clk=w)
    #     sleep(2)
