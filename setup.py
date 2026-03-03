"""Setup script for arduino-led-control package."""
from setuptools import setup, find_packages
from setuptools.command.build_py import build_py
import subprocess
import os
import shutil


class BuildWithArduino(build_py):
    """Custom build command that compiles Arduino sketches."""
    
    def run(self):
        """Build the package and compile Arduino sketches."""
        # Compile Arduino sketch
        self.compile_arduino_sketch()
        # Call parent build
        super().run()
    
    def compile_arduino_sketch(self):
        """Compile Arduino sketch using arduino-cli or similar."""
        print("Compiling Arduino sketch...")
        
        sketch_dir = os.path.join(os.path.dirname(__file__), 
                                   "arduino_led_control", "firmware")
        sketch_file = os.path.join(sketch_dir, "led_control.ino")
        
        if not os.path.exists(sketch_file):
            print(f"Warning: Arduino sketch not found at {sketch_file}")
            return
        
        # Build output directory
        build_dir = os.path.join(sketch_dir, "build")
        os.makedirs(build_dir, exist_ok=True)
        
        # Try to compile using arduino-cli
        try:
            cmd = [
                "arduino-cli",
                "compile",
                "-b", "arduino:avr:uno",
                "--build-path", build_dir,
                sketch_file
            ]
            result = subprocess.run(cmd, capture_output=True, text=True)
            if result.returncode != 0:
                print(f"Warning: Arduino compilation may have failed: {result.stderr}")
            else:
                print("Arduino sketch compiled successfully")
                
                # Copy compiled files to package data
                hex_file = os.path.join(build_dir, "led_control.ino.hex")
                if os.path.exists(hex_file):
                    shutil.copy(hex_file, sketch_dir)
                    print(f"Compiled sketch copied to {sketch_dir}")
        except FileNotFoundError:
            print("Warning: arduino-cli not found. Skipping Arduino compilation.")
            print("Install arduino-cli to enable automatic sketch compilation.")


setup(
    cmdclass={'build_py': BuildWithArduino},
)
