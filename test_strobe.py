#!/usr/bin/env python3

from arduino_led_control import ArduinoController
import openwave_1kb.dso1kb as dso1kb
from time import sleep


# Connect to Arduino LED Controller
led = ArduinoController("/dev/ttyACM0")

#Connecting to oscilloscope
dso = dso1kb.Dso("/dev/ttyACM1")
# dso=dso1kb.Dso("10.10.0.77:3001")

frequency_hz = 10
pulse_width_clk = 1024
high_value = 50
low_value = 0

led.strobe(frequency_hz, pulse_width_clk, high_value, low_value)
sleep(1)

dso.write(':DISP:PNGOutput?\n')            #Send command to get image from DSO.
dso.getBlockData()
dso.ImageDecode(0)
# dso.im.show()
dso.im.save(f"strobe_test_f{frequency_hz}_w{pulse_width_clk}_h{high_value}_l{low_value}.png")

# led.off()

