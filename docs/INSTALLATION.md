# Installation Guide

## Prerequisites

- Python 3.8 or higher
- pip (Python package manager)
- Arduino board (Uno, Nano, Mega, etc.)
- USB cable for Arduino

## Installation Steps

### 1. Install from PyPI

The easiest way to install:

```bash
pip install arduino-led-control
```

### 2. Install from Source

For development or latest features:

```bash
git clone https://github.com/tdietel/arduino-led-control.git
cd arduino-led-control
pip install -e .
```

### 3. Development Installation

For development with additional tools:

```bash
git clone https://github.com/tdietel/arduino-led-control.git
cd arduino-led-control
pip install -e ".[dev]"
```

## Arduino Setup

### Step 1: Identify Your Board Type

Common Arduino boards:
- Arduino Uno (ATmega328P)
- Arduino Nano (ATmega328P)
- Arduino Mega (ATmega2560)

### Step 2: Upload Firmware

#### Option A: Using Arduino IDE (Recommended for beginners)

1. Download and install [Arduino IDE](https://www.arduino.cc/en/software)
2. Connect your Arduino via USB
3. Open Arduino IDE
4. Go to `Sketch > Include Library > Manage Libraries`
5. Locate the firmware sketch:
   ```
   arduino_led_control/firmware/led_control.ino
   ```
6. Select `Tools > Board > Arduino Uno` (or your board)
7. Select `Tools > Port > /dev/ttyUSB0` (or COM port on Windows)
8. Click `Upload` (Ctrl+U)

#### Option B: Using arduino-cli

```bash
# Install arduino-cli
# macOS
brew install arduino-cli

# Or download from: https://arduino.github.io/arduino-cli/

# Upload sketch
arduino-cli board list  # Find your board
arduino-cli compile -b arduino:avr:uno arduino_led_control/firmware/led_control.ino
arduino-cli upload -p /dev/ttyUSB0 -b arduino:avr:uno arduino_led_control/firmware/led_control.ino
```

### Step 3: Find Your Serial Port

After uploading, determine your serial port:

**Linux/macOS:**
```bash
ls /dev/tty.* /dev/ttyUSB* /dev/ttyACM*
```

**Windows (PowerShell):**
```powershell
Get-WmiObject Win32_SerialPort
```

Or use Python:
```bash
python -m serial.tools.list_ports
```

## Verify Installation

### Test Python Package

```python
from arduino_led_control import ArduinoController

# Should import without errors
print("Installation successful!")
```

### Test Arduino Connection

```python
from arduino_led_control import ArduinoController

# Replace with your serial port
controller = ArduinoController('/dev/ttyUSB0')

if controller.connect():
    print("Connected to Arduino!")
    controller.led_on(13)
    controller.disconnect()
else:
    print("Failed to connect")
```

## Troubleshooting

### "No module named 'arduino_led_control'"

```bash
# Reinstall the package
pip install --upgrade arduino-led-control
```

### "Permission denied" on Linux

```bash
# Give user permission to USB ports
sudo usermod -a -G dialout $USER
# Logout and login again for changes to take effect
```

### "Serial port not found"

1. Check USB cable connection
2. Verify board is recognized: `arduino-cli board list`
3. Try a different USB port
4. Update Arduino drivers if on Windows

### "Device or resource busy"

Another application (like Arduino IDE) may have the port open:

1. Close Arduino IDE or other serial monitor
2. Try again

## Next Steps

See [README.md](../README.md) for usage examples and API documentation.
