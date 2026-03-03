/**
 * Arduino LED Control Firmware with Timer-Based Pulse Generation
 * 
 * This sketch receives commands via serial connection to control LEDs.
 * Uses Timer1 interrupt to generate precise pulse patterns without blocking.
 * Compatible with the arduino-led-control Python module.
 */

#include <avr/interrupt.h>

// Configuration
const int LED_PIN = 13;
const long BAUD_RATE = 115200;

// Timer1 configuration (16-bit timer, ATmega328P/2560)
volatile uint16_t pulseFrequency = 2;  // Hz (default 1kHz)
volatile uint8_t pulsePattern = 0;         // 0: off, 1: single, 2: double
volatile uint8_t pulseLengthCycles = 64;  // Clock cycles per pulse (minimum 1)
volatile boolean timerEnabled = false;
volatile uint32_t pulseCount = 0;

// Port control for assembler-based pulse generation
volatile uint8_t portAddress = 0x05;       // PORTB
volatile uint8_t portMaskOn  = 0x30;       // Pin 13 (bit 5) - internal LED
volatile uint8_t portMaskOff = 0x00;       // Mask to clear (usually 0 = set bits to 0)

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
  
  // Configure Timer1 for pulse generation
  initTimer1();
  
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
    
    } else if (action == "PULSE_MODE") {
      // PULSE_MODE:pattern:frequency:length
      // pattern: 1=single, 2=double
      // frequency: Hz (1-10000)
      // length: cycles (1-255)
      int colon1 = params.indexOf(COMMAND_DELIM);
      int colon2 = params.indexOf(COMMAND_DELIM, colon1 + 1);
      
      if (colon1 != -1 && colon2 != -1) {
        uint8_t pattern = params.substring(0, colon1).toInt();
        uint16_t freq = params.substring(colon1 + 1, colon2).toInt();
        uint8_t length = params.substring(colon2 + 1).toInt();
        
        if ((pattern == 1 || pattern == 2) && freq >= 1 && freq <= 10000 && length >= 1 && length <= 255) {
          cli();  // Disable interrupts
          pulsePattern = pattern;
          pulseFrequency = freq;
          pulseLengthCycles = length;
          sei();  // Enable interrupts
          Serial.println("OK:PULSE_MODE");
        } else {
          Serial.println("ERROR:Invalid pulse parameters");
        }
      } else {
        Serial.println("ERROR:Invalid command format");
      }
    
    } else if (action == "PULSE_START") {
      cli();
      timerEnabled = true;
      pulseCount = 0;
      sei();
      startTimer1(pulseFrequency);
      Serial.println("OK:PULSE_START");
    
    } else if (action == "PULSE_STOP") {
      cli();
      timerEnabled = false;
      sei();
      stopTimer1();
      writePortOut(portMaskOff);
      Serial.println("OK:PULSE_STOP");
    
    } else if (action == "PULSE_COUNT") {
      cli();
      uint32_t count = pulseCount;
      sei();
      Serial.print("PULSE_COUNT:");
      Serial.println(count);
    
    } else if (action == "SET_PORT") {
      // SET_PORT:port_address:mask_on:mask_off
      // UNO IO addresses for 'out': PORTB=0x05, PORTC=0x08, PORTD=0x0B
      // mask_on: bits to set HIGH for pulse
      // mask_off: bits to clear LOW (typically 0)
      int colon1 = params.indexOf(COMMAND_DELIM);
      int colon2 = params.indexOf(COMMAND_DELIM, colon1 + 1);
      
      if (colon1 != -1 && colon2 != -1) {
        // Parse hex values
        uint8_t port = (uint8_t)strtol(params.substring(0, colon1).c_str(), NULL, 16);
        uint8_t maskOn = (uint8_t)strtol(params.substring(colon1 + 1, colon2).c_str(), NULL, 16);
        uint8_t maskOff = (uint8_t)strtol(params.substring(colon2 + 1).c_str(), NULL, 16);
        
        cli();
        portAddress = port;
        portMaskOn = maskOn;
        portMaskOff = maskOff;
        sei();
        Serial.println("OK:SET_PORT");
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

/**
 * Initialize Timer1 for pulse generation
 * Sets up the timer without starting it
 */
void initTimer1() {
  cli();  // Disable global interrupts
  
  // Clear Timer1 control registers
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;
  
  // Timer1 will be configured on-demand when pulse mode is started
  // Using CTC (Clear Timer on Compare) mode for precise frequency control
  
  sei();  // Enable global interrupts
}

/**
 * Start Timer1 with the specified frequency
 * frequency: desired frequency in Hz (1-10000)
 */
void startTimer1(uint16_t frequency) {
  // Calculate compare value: F_CPU / (2 * prescaler * frequency)
  // For 16MHz and prescaler=8: compare = 16000000 / (2 * 8 * frequency) - 1
  
  uint32_t compareValue = (F_CPU / (2 * 8 * frequency)) - 1;
  
  // Clamp to 16-bit range
  if (compareValue > 65535) compareValue = 65535;
  
  cli();
  
  // CTC mode (WGM12), prescaler = 8
  TCCR1A = 0x00;
  TCCR1B = 0x0A;  // WGM12=1, CS11=1 (prescaler /8)
  TCNT1 = 0;
  OCR1A = (uint16_t)compareValue;
  
  // Enable timer compare interrupt
  TIMSK1 = 0x02;  // OCIE1A = 1
  
  sei();
}

/**
 * Stop Timer1
 */
void stopTimer1() {
  cli();
  TCCR1B = 0x00;  // Stop timer
  TIMSK1 = 0x00;  // Disable interrupts
  sei();
}

/**
 * Timer1 Compare Match Interrupt Handler
 * Called at the frequency specified by the pulse parameters
 * Generates the pulse pattern
 */
ISR(TIMER1_COMPA_vect) {
  if (!timerEnabled) {
    return;
  }
  
  // Generate pulse pattern
  if (pulsePattern == 1) {
    // Single pulse
    generateSinglePulse();
  } else if (pulsePattern == 2) {
    // Double pulse
    generateDoublePulse();
  }
  
  pulseCount++;
}

/**
 * Write an 8-bit value to a selected AVR port using 'out'.
 * Supported UNO IO addresses: PORTB=0x05, PORTC=0x08, PORTD=0x0B.
 */
inline void writePortOut(uint8_t value) {
  switch (portAddress) {
    case 0x05:
      __asm__ __volatile__("out 0x05, %0\n" : : "r" (value));
      break;
    case 0x08:
      __asm__ __volatile__("out 0x08, %0\n" : : "r" (value));
      break;
    case 0x0B:
      __asm__ __volatile__("out 0x0B, %0\n" : : "r" (value));
      break;
    default:
      // Unsupported port for 'out' on this target.
      break;
  }
}

/**
 * Generate a single pulse using inline assembler
 * Uses direct port access with 'out' instruction for cycle-accurate timing
 */
inline void generateSinglePulse() {
  uint8_t cycles = pulseLengthCycles;
  uint8_t maskOn = portMaskOn;
  
  // Set pins HIGH using 'out' instruction
  writePortOut(maskOn);
  
  // Delay for specified number of clock cycles
  __asm__ __volatile__ (
    "1: dec %0\n"      // 1 cycle: decrement counter
    "   brne 1b\n"     // 1/2 cycles: branch if not equal
    : "=r" (cycles)
    : "0" (cycles)
  );
  
  // Set pins LOW using 'out' instruction
  uint8_t maskOff = portMaskOff;
  writePortOut(maskOff);
}

/**
 * Generate a double pulse using inline assembler
 * Two pulses with a gap between them, using direct port access
 */
inline void generateDoublePulse() {
  uint8_t cycles = pulseLengthCycles;
  uint8_t gap = pulseLengthCycles / 2;
  uint8_t maskOn = portMaskOn;
  uint8_t maskOff = portMaskOff;
  
  // First pulse HIGH
  writePortOut(maskOn);
  
  // First pulse delay
  __asm__ __volatile__ (
    "1: dec %0\n"
    "   brne 1b\n"
    : "=r" (cycles)
    : "0" (cycles)
  );
  
  // First pulse LOW
  writePortOut(maskOff);
  
  // Gap between pulses
  __asm__ __volatile__ (
    "1: dec %0\n"
    "   brne 1b\n"
    : "=r" (gap)
    : "0" (gap)
  );
  
  // Second pulse HIGH
  cycles = pulseLengthCycles;
  writePortOut(maskOn);
  
  // Second pulse delay
  __asm__ __volatile__ (
    "1: dec %0\n"
    "   brne 1b\n"
    : "=r" (cycles)
    : "0" (cycles)
  );
  
  // Second pulse LOW
  writePortOut(maskOff);
}
