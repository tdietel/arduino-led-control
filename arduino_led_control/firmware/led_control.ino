/**
 * Arduino LED Control Firmware with Timer-Based Pulse Generation
 * 
 * This sketch receives commands via serial connection to control LEDs.
 * Uses Timer1 interrupt to generate precise pulse patterns without blocking.
 * Compatible with the arduino-led-control Python module.
 */

#include <avr/interrupt.h>
#include <Adafruit_INA219.h>

// Configuration
// const int LED_PIN = 13;
const long BAUD_RATE = 115200;

// Timer1 configuration (16-bit timer, ATmega328P/2560)
volatile uint16_t pulseDelay = 2;           // Hz (default 1kHz)
volatile uint8_t  pulsePattern = 1;         // 0: off, 1: single, 2: double
volatile uint8_t  pulsePort = 0;            // Port number for pulse generation (0=PORTB, 1=PORTC, 2=PORTD)
volatile uint8_t  pulsePin = 0;             // Bit mask for the pin to toggle
volatile uint16_t pulseWidth = 0;       // Pulse width in microseconds (default 100us)

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

// void setupTimer1();
// void interruptHandler();

void startStrobe(uint16_t width);
void stopStrobe();
void generateSinglePulse(uint16_t width);
void generate_single_pulse_clk(uint16_t width);
void generate_single_pulse_(uint8_t steps);

Adafruit_INA219 ina219;

void setup() {
  // Initialize serial communication
  Serial.begin(BAUD_RATE);
  
  // Initialize LED pins
  pinMode(12, OUTPUT);
  digitalWrite(12, HIGH);
  
  // Configure Timer1 for pulse generation
  // setupTimer1();
  
  ina219.begin();

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
      int width = params.toInt();
      startStrobe(width);
      Serial.println("OK:STROBE_START");
    } else if (action == "STROBE_STOP") {
      stopStrobe();
      Serial.println("OK:STROBE_STOP");
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
  } else if (command == "READVI") {
      float voltage = ina219.getBusVoltage_V();
      float current = ina219.getCurrent_mA();
      Serial.print("OK:READVI:");
      Serial.print(voltage, 3);
      Serial.print(":");
      Serial.println(current, 3);
  } else {
    Serial.println("ERROR:Unknown command");
  }
}

void startStrobe(uint16_t width) {
  pulseWidth = width;
  cli();

  // Stop Timer1 while configuring.
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;

  // Configure Timer1 in CTC mode (WGM12=1), no prescaler (CS10=1).
  TCCR1B = (1 << WGM12) | (1 << CS12);

  // Compare value for interrupt rate derived from `pulseDelay`.
  // Keep formula from current sketch behavior.
  OCR1A = F_CPU / (2 * pulseDelay);

  // Enable Timer1 compare match A interrupt.
  TIMSK1 = (1 << OCIE1A);

  sei();
}

void stopStrobe() {
  cli();
  // Disable Timer1 interrupts and stop the timer.
  TIMSK1 = 0;
  TCCR1B = 0;
  sei();
}

ISR(TIMER1_COMPA_vect) {
  switch(pulsePattern) {
    case 1: // Single pulse
      generateSinglePulse(pulseWidth);
      break;
          
    // case 2: // Double pulse
    //   for (int i = 0; i < 2; i++) {
    //     PORTB |= (1 << 5);  // Set pin high
    //     delayMicroseconds(100); // Short pulse duration
    //     PORTB &= ~(1 << 5); // Set pin low
    //     delayMicroseconds(100); // Short gap between pulses
    //   }
    //   break;
      
    default: // No pulse
      break;
  }
}

void generateSinglePulse(uint16_t width) {

  if (width == 0) {
    return;
  } else if (width < 16) {
    generate_single_pulse_clk(width);
  } else if (width < 256) {
    generate_single_pulse_250ns(width);
  } else {
    // For longer pulses, use digitalWrite and delay
    digitalWrite(12, HIGH);
    delayMicroseconds(width/16);
    digitalWrite(12, LOW);
  }
}

void generate_single_pulse_clk(uint16_t width) {
  cli();

  switch(width){
    case 0:
      break;

    case 1:
      __asm__ __volatile__(
        "sbi %[port], %[pin]\n\t" 
        "cbi %[port], %[pin]\n\t"
        :
        : [port] "I" (_SFR_IO_ADDR(PORTB)), [pin] "I" (4)
      );
      break;

    case 2:
      __asm__ __volatile__(
        "sbi %[port], %[pin]\n\t" 
        "nop\n\t"
        "cbi %[port], %[pin]\n\t"
        :
        : [port] "I" (_SFR_IO_ADDR(PORTB)), [pin] "I" (4)
      );
      break;

    case 3:
      __asm__ __volatile__(
        "sbi %[port], %[pin]\n\t" 
        "nop\n\tnop\n\t"
        "cbi %[port], %[pin]\n\t"
        :
        : [port] "I" (_SFR_IO_ADDR(PORTB)), [pin] "I" (4)
      );
      break;
   
    case 4:
      __asm__ __volatile__(
        "sbi %[port], %[pin]\n\t" 
        "nop\n\tnop\n\tnop\n\t"
        "cbi %[port], %[pin]\n\t"
        :
        : [port] "I" (_SFR_IO_ADDR(PORTB)), [pin] "I" (4)
      );
      break;

      case 5:
        __asm__ __volatile__(
        "sbi %[port], %[pin]\n\t"
        "nop\n\tnop\n\tnop\n\tnop\n\t"
        "cbi %[port], %[pin]\n\t"
        :
        : [port] "I" (_SFR_IO_ADDR(PORTB)), [pin] "I" (4)
        );
        break;
      case 6:
        __asm__ __volatile__(
        "sbi %[port], %[pin]\n\t"
        "nop\n\tnop\n\tnop\n\tnop\n\tnop\n\t"
        "cbi %[port], %[pin]\n\t"
        :
        : [port] "I" (_SFR_IO_ADDR(PORTB)), [pin] "I" (4)
        );
        break;
      case 7:
        __asm__ __volatile__(
        "sbi %[port], %[pin]\n\t"
        "nop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\t"
        "cbi %[port], %[pin]\n\t"
        :
        : [port] "I" (_SFR_IO_ADDR(PORTB)), [pin] "I" (4)
        );
        break;
      case 8:
        __asm__ __volatile__(
        "sbi %[port], %[pin]\n\t"
        "nop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\t"
        "cbi %[port], %[pin]\n\t"
        :
        : [port] "I" (_SFR_IO_ADDR(PORTB)), [pin] "I" (4)
        );
        break;
      case 9:
        __asm__ __volatile__(
        "sbi %[port], %[pin]\n\t"
        "nop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\t"
        "cbi %[port], %[pin]\n\t"
        :
        : [port] "I" (_SFR_IO_ADDR(PORTB)), [pin] "I" (4)
        );
        break;
      case 10:
        __asm__ __volatile__(
        "sbi %[port], %[pin]\n\t"
        "nop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\t"
        "cbi %[port], %[pin]\n\t"
        :
        : [port] "I" (_SFR_IO_ADDR(PORTB)), [pin] "I" (4)
        );
        break;
      case 11:
        __asm__ __volatile__(
        "sbi %[port], %[pin]\n\t"
        "nop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\t"
        "cbi %[port], %[pin]\n\t"
        :
        : [port] "I" (_SFR_IO_ADDR(PORTB)), [pin] "I" (4)
        );
        break;
      case 12:
        __asm__ __volatile__(
        "sbi %[port], %[pin]\n\t"
        "nop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\t"
        "cbi %[port], %[pin]\n\t"
        :
        : [port] "I" (_SFR_IO_ADDR(PORTB)), [pin] "I" (4)
        );
        break;
      case 13:
        __asm__ __volatile__(
        "sbi %[port], %[pin]\n\t"
        "nop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\t"
        "cbi %[port], %[pin]\n\t"
        :
        : [port] "I" (_SFR_IO_ADDR(PORTB)), [pin] "I" (4)
        );
        break;
      case 14:
        __asm__ __volatile__(
        "sbi %[port], %[pin]\n\t"
        "nop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\t"
        "cbi %[port], %[pin]\n\t"
        :
        : [port] "I" (_SFR_IO_ADDR(PORTB)), [pin] "I" (4)
        );
        break;
      case 15:
        __asm__ __volatile__(
        "sbi %[port], %[pin]\n\t"
        "nop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\t"
        "cbi %[port], %[pin]\n\t"
        :
        : [port] "I" (_SFR_IO_ADDR(PORTB)), [pin] "I" (4)
        );
        break;
  }

  // // Set pin 12 (PORTB4) high, wait 1 cycle, then set low using inline assembly
  // asm volatile(
  //   "sbi %[port], %[pin]\n\t"   // Set PORTB4 high
  //   "nop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\t"                // Wait 1 cycle
  //   "cbi %[port], %[pin]\n\t"   // Set PORTB4 low
  //   :
  //   : [port] "I" (_SFR_IO_ADDR(PORTB)), [pin] "I" (4)
  // );


  sei();
}

void generate_single_pulse_250ns(uint8_t steps) {
  // Save current Timer2 settings
  uint8_t oldTCCR2A = TCCR2A;
  uint8_t oldTCCR2B = TCCR2B;
  uint8_t oldTCNT2  = TCNT2;

  // Stop Timer2
  TCCR2B = 0;
  // Set Timer2 to normal mode, no prescaler
  TCCR2A = 0;
  TCCR2B = (1 << CS20); // prescaler = 1
  TCNT2 = 0;

  uint8_t target = steps * 4;

  // Wait for TCNT2 to reach target (inline asm)
  __asm__ __volatile__ (
    "sbi %[port], %[bit]\n\t"
    "1:\n\t"
    "lds r24, %[tcnt2]\n\t"
    "cp r24, %[target]\n\t"
    "brlo 1b\n\t"
    "cbi %[port], %[bit]\n\t"
    :
    : [port] "I" (_SFR_IO_ADDR(PORTB)), [bit] "I" (4), [tcnt2] "m" (TCNT2), [target] "r" (target)
    : "r24"
  );

  // Restore Timer2 settings
  TCCR2A = oldTCCR2A;
  TCCR2B = oldTCCR2B;
  TCNT2  = oldTCNT2;
}