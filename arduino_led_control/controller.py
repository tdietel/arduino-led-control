"""Arduino controller module for serial communication with Arduino devices."""

from __future__ import annotations

import json
from click import command
import serial
from typing import List, Optional, Tuple
from serial.tools import list_ports

class ArduinoController:
    """Control LED and other components on Arduino devices via serial communication."""
    
    def __init__(self, port: Optional[str] = None, baudrate: int = 115200, timeout: float = 1.0):
        """Initialize Arduino controller.
        
        Args:
            port: Serial port (e.g., '/dev/ttyUSB0' or 'COM3'). If omitted,
                auto-detection is used.
            baudrate: Serial communication speed (default: 115200)
            timeout: Serial read timeout in seconds (default: 1.0)
        """
        self.port = port or self.auto_detect_port()
        self.baudrate = baudrate
        self.timeout = timeout
        self.serial: Optional[serial.Serial] = None

        self.connect()

    @staticmethod
    def list_serial_ports() -> List[str]:
        """Return available serial device names."""
        return [p.device for p in list_ports.comports()]

    @classmethod
    def auto_detect_port(cls) -> str:
        """Auto-detect a likely Arduino serial port using pyserial."""
        ports = cls.list_serial_ports()
        if not ports:
            raise serial.SerialException("No serial ports found. Connect your Arduino and try again.")

        patterns = ['/dev/cu.usbserial', '/dev/ttyusb', '/dev/ttyacm', 'com']

        for port in ports:
            p = port.lower()
            for pattern in patterns:
                if p.startswith(pattern):
                    return port

        return ports[0]

    @classmethod
    def detect(cls, baudrate: int = 115200, timeout: float = 1.0) -> "ArduinoController":
        """Create a controller using auto-detected serial port."""
        return cls(port=None, baudrate=baudrate, timeout=timeout)
    
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
            
            max_attempts = 5
            for attempt in range(max_attempts):
                # print(f"Attempting to connect to Arduino on {self.port} (attempt {attempt + 1}/{max_attempts})...")
                self.serial.timeout = 5.0
                data = self.serial.readline().decode().strip()
                # print(f"Received on connect: {data}")
                self.serial.timeout = self.timeout

                if data.endswith("led_control.ino READY"):
                    self.serial.readall()
                    self._run_command("CONNECT")
                    return

            raise ConnectionError(f"Failed to connect to Arduino on {self.port} after {max_attempts} attempts.")    
        
        except serial.SerialException as e:
            print(f"Failed to connect to Arduino: {e}")
            return False
    
    def disconnect(self) -> None:
        """Disconnect from Arduino device."""
        if self.serial and self.serial.is_open:
            self.serial.close()

    def close(self) -> None:
        """Alias for disconnect()."""
        self.disconnect()
    
    def is_connected(self) -> bool:
        """Check if connected to Arduino.
        
        Returns:
            True if connected, False otherwise
        """
        return self.serial is not None and self.serial.is_open
    
    def led_on(self) -> bool:
        """Turn LED on.
        
        Returns:
            True if command sent successfully, False otherwise
        """
        return self._run_command(f"ON")
    
    def led_off(self) -> bool:
        """Turn LED off.
        
        Args:
            pin: LED pin number (default: 13)
        
        Returns:
            True if command sent successfully, False otherwise
        """
        return self._run_command(f"OFF")
        
    def start_strobe(self, frequency: float) -> bool:
        """Start strobe effect on LED.
        
        Args:
            pin: LED pin number
            frequency: Strobe frequency in Hz
            pulse_width_clk: Pulse width in clock cycles
        
        Returns:
            True if command sent successfully, False otherwise
        """
        # if frequency <= 0:
        #     raise ValueError("Frequency must be positive")
        # if pulse_width_us <= 0:
        #     raise ValueError("Pulse width must be positive")
        
        # pulse_width_clk = int(pulse_width_us * (F_CPU / 1_000_000))
        self._run_command(f"STROBE:{int(frequency)}")

    def start_double_strobe(self, pulse_width_clk: int, pulse_gap_clk: int, pulse_width2_clk: int) -> bool:
        """Start double strobe effect on LED.
        
        Args:
            pin: LED pin number
            frequency: Strobe frequency in Hz
            pulse_width_clk: Pulse width in clock cycles
            pulse_gap_clk: Gap between pulses in clock cycles
        
        Returns:
            True if command sent successfully, False otherwise
        """
        self._run_command(f"STROBE_START_DOUBLE:{pulse_width_clk}:{pulse_gap_clk}:{pulse_width2_clk}")

    def stop_strobe(self) -> bool:
        """Stop strobe effect on LED.
        
        Returns:
            True if command sent successfully, False otherwise
        """
        return self._run_command("STROBE_STOP")

    def set_strobe_frequency(self, frequency: float):
        self._run_command(f"SET_STROBE_FREQ:{frequency}")

    def _run_command(self, command: str) -> bool:
        """Send command to Arduino.
        
        Args:
            command: Command string to send
        
        Returns:
            True if sent successfully, False otherwise
        """
        if not self.is_connected():
            raise ConnectionError("Not connected to Arduino")
            # print("Not connected to Arduino")
            # return False
        
        self.serial.write(f"{command}\n".encode())
        response = self.serial.readline().decode().strip()

        if not response:
            raise TimeoutError("No response received from Arduino within timeout period.")

        else:
            print(f"Sent command: {command}, Received response: {response}")
            fields = response.split("|", 2)
            if len(fields) < 2:
                raise RuntimeError(f"Malformed response: {response}")
            
            if fields[0] != command:
                raise RuntimeError(f"Command mismatch: {response}")

            if fields[1] != "OK":
                raise RuntimeError(f"Runtime error: {response}")

            return '|'.join(fields[2:]) if len(fields) > 2 else None


        # response = self.read_response()
        # return response
        # except serial.SerialException as e:
        #     print(f"Error sending command: {e}")
        #     return False
        #     raise ConnectionError(f"Failed to send command: {e}")
    

    def get_status(self) -> dict:
        """Query the Arduino for its current strobe status.

        Returns a dict parsed from the firmware JSON response, e.g.::

            {"pattern": "single", "frequency_hz": 1000.0, "pulse_width_clk": 8, "pulse_width_ns": 500.0}
        """
        response = self._run_command("STATUS")
        return json.loads(response)

    def read_voltage_current(self) -> Tuple[float, float]:
        """
        Read voltage (V) and current (mA) from the INA219 sensor.

        Returns:
            Tuple[float, float]: (voltage_V, current_mA)

        Raises:
            RuntimeError: If the device returns an error or invalid response.
        """
        response = self._run_command("READVI")
        # response = self.read_response()
        if response.startswith("OK:READVI:"):
            try:
                _, _, voltage, current = response.strip().split(":")
                return float(voltage), float(current)
            except Exception as e:
                raise RuntimeError(f"Malformed response: {response}") from e
        raise RuntimeError(f"Device error: {response}")

    def read_response(self) -> Optional[str]:
        """Read response from Arduino, waiting up to timeout seconds.

        Args:
            timeout: Maximum time to wait for a response in seconds, or None to block indefinitely.

        Returns:
            Response string or None if no data available
        """

        if not self.is_connected():
            return None

        try:
            line = self.serial.readline().decode().strip()
            if not line:
                raise TimeoutError("No response received from Arduino within timeout period.")
            return line
        except serial.SerialException as e:
            if timeout is not None:
                self.serial.timeout = old_timeout
            print(f"Error reading response: {e}")
            raise TimeoutError("No response received from Arduino within timeout period.")
    
    def __enter__(self):
        """Context manager entry."""
        self.connect()
        return self
    
    def __exit__(self, exc_type, exc_val, exc_tb):
        """Context manager exit."""
        self.disconnect()
