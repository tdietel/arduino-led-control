"""Setup script for arduino-led-control package."""
from setuptools import setup, find_packages
from setuptools.command.build_py import build_py
import subprocess
import os
import shutil
import tempfile


class BuildWithArduino(build_py):
    """Custom build command that compiles Arduino sketches."""
    
    def run(self):
        """Build the package and compile Arduino sketches."""
        # Compile Arduino sketch
        self.compile_arduino_sketch()
        # Call parent build
        super().run()
    
    def compile_arduino_sketch(self):
        """Compile Arduino sketch using arduino-cli."""
        print("Compiling Arduino sketch...")
        
        sketch_dir = os.path.join(os.path.dirname(__file__), 
                                   "arduino_led_control", "firmware")
        sketch_file = os.path.join(sketch_dir, "led_control.ino")
        stale_build_dir = os.path.join(sketch_dir, "build")

        if os.path.isdir(stale_build_dir):
            shutil.rmtree(stale_build_dir, ignore_errors=True)
        
        if not os.path.exists(sketch_file):
            print(f"Warning: Arduino sketch not found at {sketch_file}")
            return
        
        # Try to compile using arduino-cli
        try:
            # arduino-cli expects <sketch_dir>/<sketch_dir_name>.ino.
            # Our source is firmware/led_control.ino, so compile from a temp
            # sketch folder named led_control to satisfy that requirement.
            with tempfile.TemporaryDirectory(prefix="arduino_sketch_") as tmp_dir:
                tmp_sketch_dir = os.path.join(tmp_dir, "led_control")
                os.makedirs(tmp_sketch_dir, exist_ok=True)
                tmp_sketch_file = os.path.join(tmp_sketch_dir, "led_control.ino")
                build_dir = os.path.join(tmp_dir, "build")
                os.makedirs(build_dir, exist_ok=True)
                shutil.copy2(sketch_file, tmp_sketch_file)

                cmd = [
                    "arduino-cli",
                    "compile",
                    "-b", "arduino:avr:uno",
                    "--build-path", build_dir,
                    tmp_sketch_dir,
                ]
                result = subprocess.run(
                    cmd,
                    capture_output=True,
                    text=True,
                    cwd=os.path.dirname(__file__),
                )

            if result.returncode != 0:
                print(f"Warning: Arduino compilation failed: {result.stderr}")
            else:
                print("Arduino sketch compiled successfully")
                
                # Copy compiled files to firmware directory
                hex_file = os.path.join(build_dir, "led_control.ino.hex")
                bin_file = os.path.join(build_dir, "led_control.ino.bin")
                
                if os.path.exists(hex_file):
                    dest_hex = os.path.join(sketch_dir, "led_control.hex")
                    shutil.copy(hex_file, dest_hex)
                    print(f"Compiled sketch (.hex) copied to {dest_hex}")
                
                if os.path.exists(bin_file):
                    dest_bin = os.path.join(sketch_dir, "led_control.bin")
                    shutil.copy(bin_file, dest_bin)
                    print(f"Compiled sketch (.bin) copied to {dest_bin}")
        except FileNotFoundError:
            print("Warning: arduino-cli not found. Skipping Arduino compilation.")
            print("Install with: brew install arduino-cli")


setup(
    cmdclass={'build_py': BuildWithArduino},
)

