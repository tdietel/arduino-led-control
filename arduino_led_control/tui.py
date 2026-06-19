#!/usr/bin/env python3
"""Hacky TUI for arduino-led-control. Don't judge me."""

import sys
import time

try:
    import colorama
    colorama.init()
    HAS_COLOR = True
except ImportError:
    HAS_COLOR = False

from .controller import ArduinoController
from .firmware import auto_detect_port


def print_header():
    if HAS_COLOR:
        print(colorama.Fore.CYAN + "=" * 50)
        print("  ARDUINO LED CONTROLLER - hacky TUI edition")
        print("=" * 50 + colorama.Style.RESET_ALL)
    else:
        print("=" * 50)
        print("  ARDUINO LED CONTROLLER - hacky TUI edition")
        print("=" * 50)


def print_menu():
    print()
    print("1. Turn LED ON")
    print("2. Turn LED OFF")
    print("3. Set brightness (DIM)")
    print("4. Start strobe")
    print("5. Set pulse parameters")
    print("6. Stop strobe")
    print("7. Get status")
    print("8. Read voltage/current")
    print("q. Quit")
    print()


def get_choice():
    return input("Choose [1-8/q]: ").strip().lower()


def get_int(prompt, default=None):
    while True:
        try:
            val = input(prompt)
            if not val and default is not None:
                return default
            return int(val)
        except ValueError:
            print("Not a number, try again.")


def get_float(prompt, default=None):
    while True:
        try:
            val = input(prompt)
            if not val and default is not None:
                return default
            return float(val)
        except ValueError:
            print("Not a number, try again.")


def main():
    """Main TUI loop. Super hacky, but works."""
    print_header()
    
    # Auto-detect or prompt for port
    try:
        port = auto_detect_port()
        if HAS_COLOR:
            print(f"{colorama.Fore.GREEN}Auto-detected port: {port}{colorama.Style.RESET_ALL}")
        else:
            print(f"Auto-detected port: {port}")
    except Exception as e:
        if HAS_COLOR:
            print(f"{colorama.Fore.RED}Could not auto-detect: {e}{colorama.Style.RESET_ALL}")
        else:
            print(f"Could not auto-detect: {e}")
        port = input("Enter serial port manually: ").strip()
    
    try:
        controller = ArduinoController(port=port, baudrate=115200, timeout=2.0)
        if HAS_COLOR:
            print(f"{colorama.Fore.GREEN}Connected!{colorama.Style.RESET_ALL}")
        else:
            print("Connected!")
    except Exception as e:
        if HAS_COLOR:
            print(f"{colorama.Fore.RED}Connection failed: {e}{colorama.Style.RESET_ALL}")
        else:
            print(f"Connection failed: {e}")
        sys.exit(1)
    
    while True:
        print_menu()
        choice = get_choice()
        
        try:
            if choice == '1':
                controller.led_on()
                print("LED ON")
            elif choice == '2':
                controller.led_off()
                print("LED OFF")
            elif choice == '3':
                level = get_int("Brightness (0-255): ", 128)
                controller.dim(level)
                print(f"Brightness set to {level}")
            elif choice == '4':
                freq = get_float("Frequency (Hz): ", 10.0)
                controller.start_strobe(freq)
                print(f"Strobe started at {freq} Hz")
            elif choice == '5':
                duration = get_int("Pulse duration (clk): ", 8)
                high = get_int("High value (0-255): ", 255)
                low = get_int("Low value (0-255): ", 0)
                controller.set_pulse(duration, high, low)
                print(f"Pulse set: duration={duration}, high={high}, low={low}")
            elif choice == '6':
                controller.stop_strobe()
                print("Strobe stopped")
            elif choice == '7':
                status = controller.get_status()
                if HAS_COLOR:
                    print(colorama.Fore.YELLOW + "Status:")
                    for k, v in status.items():
                        print(f"  {k}: {v}")
                    print(colorama.Style.RESET_ALL)
                else:
                    print("Status:")
                    for k, v in status.items():
                        print(f"  {k}: {v}")
            elif choice == '8':
                try:
                    voltage, current = controller.read_voltage_current()
                    print(f"Voltage: {voltage:.2f} V, Current: {current:.2f} mA")
                except Exception as e:
                    print(f"Error reading sensor: {e}")
            elif choice in ('q', 'quit', 'exit'):
                break
            else:
                print("Invalid choice. Try again.")
        except Exception as e:
            if HAS_COLOR:
                print(f"{colorama.Fore.RED}Error: {e}{colorama.Style.RESET_ALL}")
            else:
                print(f"Error: {e}")
        
        time.sleep(0.5)  # brief pause so user can see output
    
    controller.close()
    print("Bye!")


if __name__ == "__main__":
    main()
