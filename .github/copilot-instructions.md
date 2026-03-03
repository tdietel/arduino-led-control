<!-- VS Code Copilot Custom Instructions for arduino-led-control project -->

## Project Overview

Arduino LED Control is a PyPI-publishable Python module that provides an interface to control Arduino devices with embedded LED control firmware. The package includes:

- Python serial communication library (pyserial-based)
- Pre-compiled Arduino firmware for ATmega-based boards
- Wheel distribution with embedded firmware binaries
- Build system to compile Arduino sketches during packaging
- Full test suite and documentation

## Project Structure

```
arduino-led-control/
├── arduino_led_control/          # Main Python package
│   ├── __init__.py              # Package exports and version
│   ├── controller.py            # ArduinoController serial interface
│   └── firmware/                # Arduino firmware directory
│       └── led_control.ino      # Main Arduino sketch
├── tests/                        # Unit tests (pytest-based)
├── docs/                         # Documentation (INSTALLATION, ARCHITECTURE)
├── pyproject.toml               # Modern Python project config
├── setup.py                     # Build with custom Arduino compilation
├── MANIFEST.in                  # Includes firmware files in distribution
├── README.md                    # User guide and API reference
└── LICENSE                      # MIT License
```

## Key Technologies

- **Python**: 3.8+ with type hints
- **Serial Communication**: pyserial library
- **Arduino**: C++ sketches for ATmega328P/2560
- **Packaging**: setuptools with custom build backend
- **Distribution**: PyPI wheel format with binary firmware
- **Testing**: pytest with unittest.mock
- **Code Quality**: black (formatting), flake8 (linting), mypy (typing)

## Development Guidelines

### Adding Python Features

1. Update `controller.py` with new methods following existing patterns
2. Add corresponding unit tests in `tests/test_controller.py`
3. Update `README.md` API reference section
4. Update version in `pyproject.toml` if needed

### Adding Arduino Features

1. Add commands to `firmware/led_control.ino`
2. Implement corresponding Python methods in `ArduinoController`
3. Follow Arduino command protocol: `COMMAND:PARAM1:PARAM2\n`
4. Document new commands in ARCHITECTURE.md

### Testing

- Run: `pytest tests/`
- Use mocks for serial port testing
- Test both success and failure paths
- Mock serial.Serial to avoid hardware dependency

### Code Style

- Python: Follow PEP 8, format with black
- Arduino: Follow Arduino style guide
- Add docstrings to all public methods
- Use type hints in Python code

## Common Tasks

### Build and Package

```bash
# Build wheel with compiled firmware
python -m build

# Or directly with setuptools
python setup.py bdist_wheel
```

### Test

```bash
# Run all tests
pytest

# Run with coverage
pytest --cov=arduino_led_control
```

### Format and Lint

```bash
black arduino_led_control/
flake8 arduino_led_control/
mypy arduino_led_control/
```

### Compile Arduino Sketch

```bash
# Requires arduino-cli to be installed
arduino-cli compile -b arduino:avr:uno arduino_led_control/firmware/led_control.ino
```

### Upload to Arduino

```bash
arduino-cli upload -p /dev/ttyUSB0 -b arduino:avr:uno arduino_led_control/firmware/led_control.ino
```

## Serial Protocol

The package uses a simple text-based serial protocol:

**Command Format:** `COMMAND:PARAM1:PARAM2\n`

**Supported Commands:**
- `LED_ON:pin` - Turn on LED
- `LED_OFF:pin` - Turn off LED
- `BRIGHTNESS:pin:value` - Set PWM (0-255)
- `PING` - Connectivity check
- `STATUS` - Device status

**Response Format:** `OK:COMMAND` or `ERROR:REASON`

## Dependencies Management

### Runtime
- pyserial>=3.5 (serial communication)

### Build
- setuptools>=45
- wheel
- setuptools_scm[toml]>=6.2

### Development
- pytest>=7.0
- black>=22.0
- flake8>=4.0
- mypy>=0.950

### Optional
- arduino-cli (for sketch compilation from source)

## Future Extensions

Possible enhancements:
- Support for multiple Arduino boards (Uno, Mega, STM32, etc.)
- I2C/SPI support for additional sensors
- Real-time status monitoring
- Firmware update mechanism
- Web dashboard for remote control
- Alternative communication protocols (USB HID, Bluetooth)

## Resources

- **Arduino**: https://www.arduino.cc/
- **pyserial**: https://github.com/pyserial/pyserial
- **setuptools**: https://setuptools.pypa.io/
- **PyPI**: https://pypi.org/
