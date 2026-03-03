/**
 * Arduino LED Control Firmware
 * 
 * This sketch receives commands via serial connection to control LEDs.
 * Compatible with the arduino-led-control Python module.
 */

// Configuration
const int LED_PIN = 13;
const long BAUD_RATE = 9600;

// Variables
String inputBuffer = "";
const char COMMAND_DELIM = ':';
const char LINE_ENDING = '\n';

void setup() {
  // Initialize serial communication
  Serial.begin(BAUD_RATE);
  
  // Initialize LED pins
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  
  // Startup signal
  Serial.println("READY");
}

void loop() {
  // Check for incoming serial data
  if (Serial.available() > 0) {
    char c = Serial.read();
    
    if (c == LINE_ENDING) {
      // Process complete command
      processCommand(inputBuffer);
      inputBuffer = "";
    } else if (c != '\r') {
      // Accumulate command (ignore carriage returns)
      inputBuffer += c;
    }
  }
}

/**
 * Process incoming command
 * 
 * Supported commands:
 * - LED_ON:pin     : Turn on LED at pin
 * - LED_OFF:pin    : Turn off LED at pin
 * - BRIGHTNESS:pin:value : Set PWM brightness (0-255)
 */
void processCommand(String command) {
  if (command.length() == 0) {
    return;
  }
  
  // Parse command
  int firstColon = command.indexOf(COMMAND_DELIM);
  
  if (firstColon == -1) {
    // No parameters
    handleSimpleCommand(command);
  } else {
    // Commands with parameters
    String action = command.substring(0, firstColon);
    String params = command.substring(firstColon + 1);
    
    if (action == "LED_ON") {
      int pin = params.toInt();
      digitalWrite(pin, HIGH);
      Serial.println("OK:LED_ON");
      
    } else if (action == "LED_OFF") {
      int pin = params.toInt();
      digitalWrite(pin, LOW);
      Serial.println("OK:LED_OFF");
      
    } else if (action == "BRIGHTNESS") {
      // Parse pin and value
      int secondColon = params.indexOf(COMMAND_DELIM);
      if (secondColon != -1) {
        int pin = params.substring(0, secondColon).toInt();
        int value = params.substring(secondColon + 1).toInt();
        
        if (value >= 0 && value <= 255) {
          analogWrite(pin, value);
          Serial.println("OK:BRIGHTNESS");
        } else {
          Serial.println("ERROR:Invalid brightness value");
        }
      } else {
        Serial.println("ERROR:Invalid command format");
      }
    } else {
      Serial.println("ERROR:Unknown command");
    }
  }
}

/**
 * Handle simple commands without parameters
 */
void handleSimpleCommand(String command) {
  if (command == "PING") {
    Serial.println("PONG");
  } else if (command == "STATUS") {
    Serial.println("STATUS:OK");
  } else {
    Serial.println("ERROR:Unknown command");
  }
}
