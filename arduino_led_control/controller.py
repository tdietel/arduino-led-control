"""Arduino controller module for serial communication with Arduino devices."""

import serial
import time
from typing import Optional


class ArduinoController:
    """Control LED and other components on Arduino devices via serial communication."""
    
    def __init__(self, port: str, baudrate: int = 9600, timeout: float = 1.0):
        """Initialize Arduino controller.
        
        Args:
            port: Serial port (e.g., '/dev/ttyUSB0' or 'COM3')
            baudrate: Serial communication speed (default: 9600)
            timeout: Serial read timeout in seconds (default: 1.0)
        """
        self.port = port
        self.baudrate = baudrate
        self.timeout = timeout
        self.serial: Optional[serial.Serial] = None
    
    def connect(self) -> bool:
        """Connect to Arduino device.
        
        Returns:
            True if connection successful, False otherwise
        """
        try:
            self.serial = serial.Serial(
                port=self.port,
                baudrate=self.baudrate,
                timeout=self.timeout
            )
            time.sleep(2)  # Wait for Arduino to initialize
            return True
        except serial.SerialException as e:
            print(f"Failed to connect to Arduino: {e}")
            return False
    
    def disconnect(self) -> None:
        """Disconnect from Arduino device."""
        if self.serial and self.serial.is_open:
            self.serial.close()
    
    def is_connected(self) -> bool:
        """Check if connected to Arduino.
        
        Returns:
            True if connected, False otherwise
        """
        return self.serial is not None and self.serial.is_open
    
    def led_on(self, pin: int = 13) -> bool:
        """Turn LED on.
        
        Args:
            pin: LED pin number (default: 13)
        
        Returns:
            True if command sent successfully, False otherwise
        """
        return self._send_command(f"LED_ON:{pin}")
    
    def led_off(self, pin: int = 13) -> bool:
        """Turn LED off.
        
        Args:
            pin: LED pin number (default: 13)
        
        Returns:
            True if command sent successfully, False otherwise
        """
        return self._send_command(f"LED_OFF:{pin}")
    
    def set_brightness(self, pin: int, value: int) -> bool:
        """Set LED brightness using PWM.
        
        Args:
            pin: LED pin number
            value: Brightness value (0-255)
        
        Returns:
            True if command sent successfully, False otherwise
        """
        if not 0 <= value <= 255:
            raise ValueError("Brightness value must be between 0 and 255")
        return self._send_command(f"BRIGHTNESS:{pin}:{value}")
    
    def _send_command(self, command: str) -> bool:
        """Send command to Arduino.
        
        Args:
            command: Command string to send
        
        Returns:
            True if sent successfully, False otherwise
        """
        if not self.is_connected():
            print("Not connected to Arduino")
            return False
        
        try:
            self.serial.write(f"{command}\n".encode())
            return True
        except serial.SerialException as e:
            print(f"Error sending command: {e}")
            return False
    
    def read_response(self) -> Optional[str]:
        """Read response from Arduino.
        
        Returns:
            Response string or None if no data available
        """
        if not self.is_connected():
            return None
        
        try:
            if self.serial.in_waiting > 0:
                response = self.serial.readline().decode().strip()
                return response
        except serial.SerialException as e:
            print(f"Error reading response: {e}")
        
        return None
    
    def __enter__(self):
        """Context manager entry."""
        self.connect()
        return self
    
    def __exit__(self, exc_type, exc_val, exc_tb):
        """Context manager exit."""
        self.disconnect()
