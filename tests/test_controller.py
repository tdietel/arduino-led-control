"""Tests for ArduinoController class."""

import unittest
from unittest.mock import Mock, patch, MagicMock
from arduino_led_control import ArduinoController


class TestArduinoController(unittest.TestCase):
    """Test cases for ArduinoController."""
    
    def setUp(self):
        """Set up test fixtures."""
        self.controller = ArduinoController('/dev/ttyUSB0')
    
    def test_initialization(self):
        """Test controller initialization."""
        self.assertEqual(self.controller.port, '/dev/ttyUSB0')
        self.assertEqual(self.controller.baudrate, 9600)
        self.assertEqual(self.controller.timeout, 1.0)
        self.assertIsNone(self.controller.serial)
    
    def test_is_connected_when_not_connected(self):
        """Test is_connected returns False when not connected."""
        self.assertFalse(self.controller.is_connected())
    
    @patch('serial.Serial')
    def test_connect_success(self, mock_serial):
        """Test successful connection."""
        mock_instance = MagicMock()
        mock_serial.return_value = mock_instance
        
        result = self.controller.connect()
        
        self.assertTrue(result)
        self.assertTrue(self.controller.is_connected())
    
    @patch('serial.Serial')
    def test_connect_failure(self, mock_serial):
        """Test failed connection."""
        import serial
        mock_serial.side_effect = serial.SerialException("Port not found")
        
        result = self.controller.connect()
        
        self.assertFalse(result)
        self.assertFalse(self.controller.is_connected())
    
    @patch('serial.Serial')
    def test_led_on(self, mock_serial):
        """Test LED on command."""
        mock_instance = MagicMock()
        mock_serial.return_value = mock_instance
        
        self.controller.connect()
        result = self.controller.led_on(13)
        
        self.assertTrue(result)
        mock_instance.write.assert_called()
    
    @patch('serial.Serial')
    def test_led_off(self, mock_serial):
        """Test LED off command."""
        mock_instance = MagicMock()
        mock_serial.return_value = mock_instance
        
        self.controller.connect()
        result = self.controller.led_off(13)
        
        self.assertTrue(result)
        mock_instance.write.assert_called()
    
    @patch('serial.Serial')
    def test_set_brightness(self, mock_serial):
        """Test brightness command."""
        mock_instance = MagicMock()
        mock_serial.return_value = mock_instance
        
        self.controller.connect()
        result = self.controller.set_brightness(11, 128)
        
        self.assertTrue(result)
        mock_instance.write.assert_called()
    
    def test_set_brightness_invalid_value(self):
        """Test brightness command with invalid value."""
        with self.assertRaises(ValueError):
            self.controller.set_brightness(11, 256)
    
    @patch('serial.Serial')
    def test_context_manager(self, mock_serial):
        """Test context manager usage."""
        mock_instance = MagicMock()
        mock_serial.return_value = mock_instance
        
        with ArduinoController('/dev/ttyUSB0') as controller:
            self.assertTrue(controller.is_connected())
        
        mock_instance.close.assert_called_once()


if __name__ == '__main__':
    unittest.main()
