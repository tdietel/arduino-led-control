# Arduino LED Control

A Python module for controlling Arduino devices with LED control firmware. This package includes the compiled Arduino sketch and Python code for easy communication with Arduino boards.

## Features

- Serial communication with Arduino devices
- LED on/off control
- PWM brightness control
- Easy-to-use Python interface
- PyPI package distribution
- Pre-compiled Arduino firmware included

## Installation

### From PyPI

```bash
pip install arduino-led-control
```

### From Source

```bash
git clone https://github.com/tdietel/arduino-led-control.git
cd arduino-led-control
pip install -e .
```

## Requirements

- Python 3.8+
- pyserial
- Arduino device with compatible sketch

### Optional

- `arduino-cli` for compiling sketches from source

## Usage

### Basic Usage

```python
from arduino_led_control import ArduinoController

# Create controller
controller = ArduinoController('/dev/ttyUSB0')

# Connect to Arduino
if controller.connect():
    # Turn LED on
    controller.led_on(13)
    
    # Set brightness
    controller.set_brightness(11, 200)
    
    # Disconnect
    controller.disconnect()
```

### Using Context Manager

```python
from arduino_led_control import ArduinoController

with ArduinoController('/dev/ttyUSB0') as controller:
    controller.led_on(13)
    controller.set_brightness(11, 128)
```

## Arduino Setup

### Finding Your Serial Port

**Linux/Mac:**
```bash
ls /dev/tty*
# Look for /dev/ttyUSB0, /dev/ttyUSB1, /dev/ttyACM0, etc.
```

**Windows:**
```bash
# Check Device Manager or use:
python -m serial.tools.list_ports
```

### Uploading Sketch

The Arduino sketch is included in the package. To upload it:

1. Open Arduino IDE
2. Go to `Tools > Board` and select your board (e.g., Arduino Uno)
3. Go to `Tools > Port` and select your port
4. Open the sketch from `arduino_led_control/firmware/led_control.ino`
5. Click Upload

## API Reference

### ArduinoController

#### `__init__(port, baudrate=9600, timeout=1.0)`
Initialize the controller.

**Parameters:**
- `port` (str): Serial port path
- `baudrate` (int): Communication speed (default: 9600)
- `timeout` (float): Read timeout in seconds

#### `connect() -> bool`
Connect to Arduino device.

#### `disconnect() -> None`
Disconnect from Arduino device.

#### `is_connected() -> bool`
Check connection status.

#### `led_on(pin=13) -> bool`
Turn LED on.

#### `led_off(pin=13) -> bool`
Turn LED off.

#### `set_brightness(pin, value) -> bool`
Set LED brightness using PWM (0-255).

#### `read_response() -> Optional[str]`
Read response from Arduino.

## Testing

Run tests with pytest:

```bash
pytest tests/
```

## Development

### Setup Development Environment

```bash
pip install -e ".[dev]"
```

### Run Linting

```bash
black arduino_led_control/
flake8 arduino_led_control/
mypy arduino_led_control/
```

## Arduino Commands

The sketch supports the following serial commands:

| Command | Parameters | Response | Description |
|---------|-----------|----------|-------------|
| LED_ON | pin | OK:LED_ON | Turn on LED at pin |
| LED_OFF | pin | OK:LED_OFF | Turn off LED at pin |
| BRIGHTNESS | pin, value (0-255) | OK:BRIGHTNESS | Set PWM brightness |
| PING | - | PONG | Ping test |
| STATUS | - | STATUS:OK | Get device status |

### Command Examples

```
LED_ON:13
LED_OFF:13
BRIGHTNESS:11:200
PING
STATUS
```

## Troubleshooting

### Connection Issues

1. Check the serial port is correct: `ls /dev/tty*`
2. Verify baud rate matches sketch (default: 9600)
3. Ensure Arduino is properly connected and powered
4. Check USB cable is not damaged

### Installation Issues

If `arduino-cli` is not found during installation, the build will continue without sketch compilation. You can still use pre-compiled sketches or compile manually using Arduino IDE.

## License

MIT License - See LICENSE file for details

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## Support

For issues and questions, please visit:
https://github.com/tdietel/arduino-led-control/issues
