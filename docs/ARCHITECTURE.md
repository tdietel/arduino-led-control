# Architecture Overview

## Project Structure

```
arduino-led-control/
├── arduino_led_control/           # Main Python package
│   ├── __init__.py               # Package initialization
│   ├── controller.py             # ArduinoController class
│   └── firmware/                 # Arduino sketch files
│       ├── led_control.ino       # Main Arduino sketch
│       └── build/                # Compiled artifacts (build-time)
├── tests/                         # Unit tests
│   ├── __init__.py
│   └── test_controller.py
├── docs/                          # Documentation
│   ├── INSTALLATION.md
│   ├── ARCHITECTURE.md           # This file
│   └── CONTRIBUTING.md           # Contribution guidelines
├── pyproject.toml                # Modern Python project config
├── setup.py                      # Build configuration with Arduino support
├── MANIFEST.in                   # Package data manifest
├── README.md                     # Project overview
├── LICENSE                       # MIT License
└── .gitignore                    # Git ignore rules
```

## Component Architecture

### 1. Python Module: `arduino_led_control`

#### `controller.py`
Main interface for communicating with Arduino devices:

- **ArduinoController**: Class managing serial communication
  - `connect()`: Establish serial connection
  - `disconnect()`: Close serial connection
  - `led_on(pin)`: Send LED on command
  - `led_off(pin)`: Send LED off command
  - `set_brightness(pin, value)`: Send PWM command
  - `_send_command(cmd)`: Internal command transmission
  - `read_response()`: Receive data from Arduino

#### `__init__.py`
Package initialization:
- Exports public API
- Manages version information
- Imports main classes

### 2. Arduino Firmware: `firmware/led_control.ino`

Microcontroller code running on Arduino:

**Command Processing Loop:**
```
Setup
  ├─ Initialize Serial (9600 baud)
  ├─ Configure LED pins
  └─ Send READY signal

Loop (continuously)
  ├─ Check for incoming serial data
  ├─ Buffer commands
  ├─ Parse and execute commands
  └─ Send responses
```

**Supported Commands:**
- `LED_ON:pin` → Turn on pin
- `LED_OFF:pin` → Turn off pin
- `BRIGHTNESS:pin:value` → PWM control
- `PING` → Connectivity test
- `STATUS` → Device status

### 3. Build System

#### `setup.py`
Custom build configuration:

**BuildWithArduino class:**
- Extends setuptools build process
- Compiles Arduino sketch using `arduino-cli`
- Packages compiled `.hex` files into wheel
- Creates distributable package with embedded firmware

#### `pyproject.toml`
Modern Python packaging specification:
- Dependencies (pyserial)
- Metadata (author, license, keywords)
- Build requirements
- Tool configurations (black, mypy)

### 4. Package Distribution

The package is distributed as a wheel (.whl) containing:
- Python source code
- Compiled Arduino firmware (.hex files)
- Documentation
- License files

Users can install with:
```bash
pip install arduino-led-control
```

## Communication Protocol

### Serial Connection
- **Baud Rate**: 9600 (configurable)
- **Data Bits**: 8
- **Stop Bits**: 1
- **Parity**: None
- **Flow Control**: None

### Command Format
```
COMMAND:PARAM1:PARAM2\n
```

Example:
```
LED_ON:13
BRIGHTNESS:11:200
```

### Response Format
```
OK:COMMAND_NAME
ERROR:DESCRIPTION
RESPONSE_DATA
```

Example:
```
OK:LED_ON
OK:BRIGHTNESS
ERROR:Invalid brightness value
```

## Data Flow

### Command Flow
```
Python Code
    ↓
ArduinoController.led_on(pin)
    ↓
_send_command("LED_ON:13")
    ↓
serial.write("LED_ON:13\n")
    ↓
[Over USB Serial Connection]
    ↓
Arduino Serial Buffer
    ↓
processCommand("LED_ON:13")
    ↓
digitalWrite(13, HIGH)
    ↓
Serial.println("OK:LED_ON")
    ↓
[Over USB Serial Connection]
    ↓
Python read_response()
    ↓
Returns "OK:LED_ON"
```

## Dependencies

### Runtime Dependencies
- **pyserial** (>=3.5): Serial port communication

### Build Dependencies
- **setuptools** (>=45): Package building
- **wheel**: Wheel distribution format
- **setuptools_scm** (>=6.2): Version management

### Optional Development Dependencies
- **pytest** (>=7.0): Testing framework
- **black** (>=22.0): Code formatting
- **flake8** (>=4.0): Linting
- **mypy** (>=0.950): Static type checking

### Optional Compilation
- **arduino-cli**: Arduino sketch compilation

## Testing Architecture

Tests are organized in `tests/` directory:

**test_controller.py:**
- Tests ArduinoController initialization
- Mocks serial connection for unit testing
- Tests command transmission
- Tests context manager functionality
- Tests error handling

Run tests:
```bash
pytest tests/
```

## PyPI Distribution

When built and published to PyPI:

1. Source files are packaged
2. Arduino sketch is compiled (if arduino-cli available)
3. Compiled `.hex` file is included in package data
4. Wheel is created with all resources
5. Users download and install with pip
6. All files including firmware are available locally

## Extensibility

The architecture is designed to be extended:

1. **New Commands**: Add methods to ArduinoController
2. **New Arduino Features**: Modify firmware sketch
3. **Alternative Boards**: Update board selection in setup.py
4. **Additional Protocols**: Add new communication modules
5. **Hardware Support**: Add board-specific configurations

## Version Management

Uses `setuptools_scm` for automatic version management:
- Version derived from git tags
- Format: `major.minor.patch`
- Automatic version bumping supported
