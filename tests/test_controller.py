"""Tests for ArduinoController class."""

import unittest
from unittest.mock import MagicMock, patch
import serial

from arduino_led_control import ArduinoController


class TestArduinoController(unittest.TestCase):

    def setUp(self):
        self.serial_patcher = patch('serial.Serial')
        self.mock_serial_cls = self.serial_patcher.start()
        self.mock_serial = MagicMock()
        self.mock_serial.is_open = True
        self.mock_serial.in_waiting = 1
        self.mock_serial.readline.return_value = b'OK\n'
        self.mock_serial_cls.return_value = self.mock_serial
        self.controller = ArduinoController('/dev/ttyUSB0')

    def tearDown(self):
        self.serial_patcher.stop()

    def test_initialization(self):
        self.assertEqual(self.controller.port, '/dev/ttyUSB0')
        self.assertEqual(self.controller.baudrate, 115200)
        self.assertEqual(self.controller.timeout, 1.0)

    def test_is_connected(self):
        self.assertTrue(self.controller.is_connected())

    def test_is_connected_when_not_connected(self):
        self.mock_serial.is_open = False
        self.assertFalse(self.controller.is_connected())

    def test_connect_failure(self):
        self.mock_serial_cls.side_effect = serial.SerialException("Port not found")
        controller = ArduinoController.__new__(ArduinoController)
        controller.port = '/dev/ttyUSB0'
        controller.baudrate = 115200
        controller.timeout = 1.0
        controller.serial = None
        result = controller.connect()
        self.assertFalse(result)
        self.assertFalse(controller.is_connected())

    def test_led_on(self):
        self.mock_serial.readline.return_value = b'OK:LED_ON\n'
        self.controller.led_on(13)
        self.mock_serial.write.assert_called_with(b'LED_ON:13\n')

    def test_led_off(self):
        self.mock_serial.readline.return_value = b'OK:LED_OFF\n'
        self.controller.led_off(13)
        self.mock_serial.write.assert_called_with(b'LED_OFF:13\n')

    def test_start_strobe(self):
        self.mock_serial.readline.return_value = b'OK:STROBE_START_SINGLE\n'
        self.controller.start_strobe(100)
        self.mock_serial.write.assert_called_with(b'STROBE_START_SINGLE:100\n')

    def test_start_double_strobe(self):
        self.mock_serial.readline.return_value = b'OK:STROBE_START_DOUBLE\n'
        self.controller.start_double_strobe(50, 100, 50)
        self.mock_serial.write.assert_called_with(b'STROBE_START_DOUBLE:50:100:50\n')

    def test_stop_strobe(self):
        self.mock_serial.readline.return_value = b'OK:STROBE_STOP\n'
        self.controller.stop_strobe()
        self.mock_serial.write.assert_called_with(b'STROBE_STOP\n')

    def test_set_strobe_frequency(self):
        self.mock_serial.readline.return_value = b'OK:SET_STROBE_FREQ\n'
        self.controller.set_strobe_frequency(1000.0)
        self.mock_serial.write.assert_called_with(b'SET_STROBE_FREQ:1000.0\n')

    def test_read_voltage_current(self):
        self.mock_serial.readline.return_value = b'OK:READVI:3.300:250.000\n'
        voltage, current = self.controller.read_voltage_current()
        self.assertAlmostEqual(voltage, 3.3)
        self.assertAlmostEqual(current, 250.0)

    def test_not_connected_raises(self):
        self.mock_serial.is_open = False
        with self.assertRaises(ConnectionError):
            self.controller.led_on(13)

    def test_context_manager(self):
        with ArduinoController('/dev/ttyUSB0') as controller:
            self.assertTrue(controller.is_connected())
        self.mock_serial.close.assert_called()


if __name__ == '__main__':
    unittest.main()
