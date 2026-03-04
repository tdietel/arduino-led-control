/**
 * Arduino LED Control Firmware with Timer-Based Pulse Generation
 * 
 * This sketch receives commands via serial connection to control LEDs.
 * Uses Timer1 interrupt to generate precise pulse patterns without blocking.
 * Compatible with the arduino-led-control Python module.
 */

#include <avr/interrupt.h>

// Configuration
// const int LED_PIN = 13;
const long BAUD_RATE = 115200;

// Timer1 configuration (16-bit timer, ATmega328P/2560)
volatile uint16_t pulseDelay = 2;           // Hz (default 1kHz)
volatile uint8_t  pulsePattern = 0;         // 0: off, 1: single, 2: double
volatile uint8_t  pulsePort = 0;            // Port number for pulse generation (0=PORTB, 1=PORTC, 2=PORTD)
volatile uint8_t  pulsePin = 0;             // Bit mask for the pin to toggle

// volatile boolean  timerEnabled = false;
// volatile uint32_t pulseCount = 0;

// Port control for assembler-based pulse generation
// volatile uint8_t portAddress = 0x05;       // PORTB
// volatile uint8_t portMaskOn  = 0x30;       // Pin 13 (bit 5) - internal LED
// volatile uint8_t portMaskOff = 0x00;       // Mask to clear (usually 0 = set bits to 0)

// Variables
String inputBuffer = "";
const char COMMAND_DELIM = ':';
const char LINE_ENDING = '\n';

void setupTimer1();
void interruptHandler();

void setup() {
  // Initialize serial communication
  Serial.begin(BAUD_RATE);
  
  // Initialize LED pins
  pinMode(12, OUTPUT);
  digitalWrite(12, HIGH);
  
  // Configure Timer1 for pulse generation
  setupTimer1();
  
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
      pinMode(pin, OUTPUT);
      digitalWrite(pin, HIGH);
      Serial.println("OK:LED_ON"); 
    } else if (action == "LED_OFF") {
      int pin = params.toInt();
      digitalWrite(pin, LOW);
      Serial.println("OK:LED_OFF");
    } else if (action == "STROBE_START") {

    } else {
      Serial.println("ERROR:Unknown command");
    }
  }
}

void setupTimer1() {
  cli();

  // Stop Timer1 while configuring.
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;

  // Configure Timer1 in CTC mode (WGM12=1), no prescaler (CS10=1).
  TCCR1B = (1 << WGM12) | (1 << CS10);

  // Compare value for interrupt rate derived from `pulseDelay`.
  // Keep formula from current sketch behavior.
  OCR1A = F_CPU / (2 * pulseDelay);

  // Enable Timer1 compare match A interrupt.
  TIMSK1 = (1 << OCIE1A);

  sei();
}

ISR(TIMER1_COMPA_vect) {
  interruptHandler();
}

void interruptHandler() {
  // Intentionally empty for now.
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
