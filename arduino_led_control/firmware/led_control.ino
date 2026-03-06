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

float             pulseFreq = 1000.;        // Frequency of pulse generation
volatile uint8_t  pulsePattern = 1;         // 0: off, 1: single, 2: double
volatile uint8_t  pulsePort = 0;            // Port number for pulse generation (0=PORTB, 1=PORTC, 2=PORTD)
volatile uint8_t  pulsePin = 0;             // Bit mask for the pin to toggle
volatile uint16_t pulseWidth = 0;           // Pulse width in clock cycles (1 cycle = 62.5 ns at 16 MHz)
volatile uint16_t pulseGap = 0;             // Gap between pulses for double pulse mode
volatile uint16_t pulseWidth2 = 0;          // Width of second pulse in double pulse mode


// Variables
String inputBuffer = "";
const char COMMAND_DELIM = ':';
const char LINE_ENDING = '\n';

// void setupTimer1();
// void interruptHandler();

void startStrobe();
void stopStrobe();
void generate_single_pulse(uint16_t width);
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
    
    char line[64];
    int pin, width, gap, width2, freq;

    size_t n = Serial.readBytesUntil('\n', line, sizeof(line)-1);
    line[n] = '\0';   // terminate

    if (strncmp(line, "PING", 4) == 0) {
      Serial.println("PONG");

    } else if (strncmp(line, "STATUS", 6) == 0) {
      Serial.println("STATUS:OK");

    } else if (strncmp(line, "READVI", 6) == 0) {
      float voltage = ina219.getBusVoltage_V();
      float current = ina219.getCurrent_mA();
      Serial.print("OK:READVI:");
      Serial.print(voltage, 3);
      Serial.print(":");
      Serial.println(current, 3);

    } else if (sscanf(line, "LED_ON:%i", &pin) == 1) {
      pinMode(pin, OUTPUT);
      digitalWrite(pin, HIGH);
      Serial.println("OK:LED_ON");

    } else if (sscanf(line, "LED_OFF:%i", &pin) == 1) {
      digitalWrite(pin, LOW);
      Serial.println("OK:LED_OFF"); 

    } else if (sscanf(line, "STROBE_START_SINGLE:%d", &width) == 1) {
      pulsePattern = 1;
      pulseWidth = width;
      startStrobe();
      Serial.println("OK:STROBE_START_SINGLE");

    } else if (sscanf(line, "SET_STROBE_FREQ:%i", &freq) == 1) {
      pulseFreq = float(freq);
      Serial.println("OK:SET_STROBE_FREQ");

    } else if (sscanf(line, "STROBE_START_DOUBLE:%d:%d:%d", &width, &gap, &width2) == 3) {
      pulsePattern = 2;
      pulseWidth = width;
      pulseGap = gap;
      pulseWidth2 = width2;
      startStrobe();
      Serial.println("OK:STROBE_START_DOUBLE");

    } else if (strncmp(line, "STROBE_STOP", 11) == 0) {
      stopStrobe();
      Serial.println("OK:STROBE_STOP");

    } else {
      Serial.println("ERROR:Unknown command: " + String(line));
    }
  }
}


void startStrobe() {
  cli();

  // Stop Timer1 while configuring.
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;

  // Configure Timer1 in CTC mode (WGM12=1), prescaler = 64
  int prescaler = 64;
  TCCR1B = (1 << WGM12) | (1 << CS11) | (1 << CS10);

  // // Configure Timer1 in CTC mode (WGM12=1), no prescaler (CS10=1).
  // int prescaler = 1;
  // TCCR1B = (1 << WGM12) | (1 << CS10);

  // Compare value for interrupt rate derived from `pulseDelay`.
  // Keep formula from current sketch behavior.
  // OCR1A = F_CPU / (2 * pulseFreq);
  OCR1A = (F_CPU / (prescaler * pulseFreq)) - 1;

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
      generate_single_pulse(pulseWidth);
      break;

    case 2: // Double pulse
      generate_double_pulse(pulseWidth, pulseGap, pulseWidth2);
      break;
      
    default: // No pulse
      break;
  }
}

void generate_single_pulse(uint16_t width) {

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

void generate_double_pulse(uint8_t width1, uint8_t gap, uint8_t width2) {
  // This function is only implemented with 250ns accuracy using Timer2, so steps should be between 1 and 15 (4 to 60 cycles)

  // // Save current Timer2 settings
  // uint8_t oldTCCR2A = TCCR2A;
  // uint8_t oldTCCR2B = TCCR2B;
  // uint8_t oldTCNT2  = TCNT2;

  // // Stop Timer2
  // TCCR2B = 0;
  // // Set Timer2 to normal mode, no prescaler
  // TCCR2A = 0;
  // TCCR2B = (1 << CS20); // prescaler = 1
  // TCNT2 = 0;

  volatile uint16_t w1 = width1 / 4;
  volatile uint16_t g = gap / 4;
  volatile uint16_t w2 = width2 / 4;

  cli();
  __asm__ __volatile__ (
    "movw r24, %[w1]\n"   // load width into r24:r25
    "movw r26, %[g]\n"  // load gap into r26:r27
    "movw r30, %[w2]\n"   // load width2 into r30:r31
    "sbi %[port], %[bit]\n\t"
    "loopA:\n"
        "sbiw r24, 1\n"
        "brne loopA\n"
    "cbi %[port], %[bit]\n\t"
    "loopB:\n"
        "sbiw r26, 1\n"
        "brne loopB\n"
    "sbi %[port], %[bit]\n\t"
    "loopC:\n"
        "sbiw r30, 1\n"
        "brne loopC\n"
    "cbi %[port], %[bit]\n\t"
    :
    : [port] "I" (_SFR_IO_ADDR(PORTB)), [bit] "I" (4)
    , [w1] "r" (w1), [g] "r" (g), [w2] "r" (w2)
    : "r24", "r25", "r26", "r27", "r30", "r31"
  );
  sei();

  // // Restore Timer2 settings
  // TCCR2A = oldTCCR2A;
  // TCCR2B = oldTCCR2B;
  // TCNT2  = oldTCNT2;
}

void generate_single_pulse_clk(uint16_t width) {
  cli();

  switch(width){
    case 0:
      break;

    case 2:
      __asm__ __volatile__(
        "sbi %[port], %[pin]\n\t" 
        "cbi %[port], %[pin]\n\t"
        :
        : [port] "I" (_SFR_IO_ADDR(PORTB)), [pin] "I" (4)
      );
      break;

    case 3:
      __asm__ __volatile__(
        "sbi %[port], %[pin]\n\t" 
        "nop\n\t"
        "cbi %[port], %[pin]\n\t"
        :
        : [port] "I" (_SFR_IO_ADDR(PORTB)), [pin] "I" (4)
      );
      break;

    case 4:
      __asm__ __volatile__(
        "sbi %[port], %[pin]\n\t" 
        "nop\n\tnop\n\t"
        "cbi %[port], %[pin]\n\t"
        :
        : [port] "I" (_SFR_IO_ADDR(PORTB)), [pin] "I" (4)
      );
      break;
   
    case 5:
      __asm__ __volatile__(
        "sbi %[port], %[pin]\n\t" 
        "nop\n\tnop\n\tnop\n\t"
        "cbi %[port], %[pin]\n\t"
        :
        : [port] "I" (_SFR_IO_ADDR(PORTB)), [pin] "I" (4)
      );
      break;

      case 6:
        __asm__ __volatile__(
        "sbi %[port], %[pin]\n\t"
        "nop\n\tnop\n\tnop\n\tnop\n\t"
        "cbi %[port], %[pin]\n\t"
        :
        : [port] "I" (_SFR_IO_ADDR(PORTB)), [pin] "I" (4)
        );
        break;
      case 7:
        __asm__ __volatile__(
        "sbi %[port], %[pin]\n\t"
        "nop\n\tnop\n\tnop\n\tnop\n\tnop\n\t"
        "cbi %[port], %[pin]\n\t"
        :
        : [port] "I" (_SFR_IO_ADDR(PORTB)), [pin] "I" (4)
        );
        break;
      case 8:
        __asm__ __volatile__(
        "sbi %[port], %[pin]\n\t"
        "nop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\t"
        "cbi %[port], %[pin]\n\t"
        :
        : [port] "I" (_SFR_IO_ADDR(PORTB)), [pin] "I" (4)
        );
        break;
      case 9:
        __asm__ __volatile__(
        "sbi %[port], %[pin]\n\t"
        "nop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\t"
        "cbi %[port], %[pin]\n\t"
        :
        : [port] "I" (_SFR_IO_ADDR(PORTB)), [pin] "I" (4)
        );
        break;
      case 10:
        __asm__ __volatile__(
        "sbi %[port], %[pin]\n\t"
        "nop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\t"
        "cbi %[port], %[pin]\n\t"
        :
        : [port] "I" (_SFR_IO_ADDR(PORTB)), [pin] "I" (4)
        );
        break;
      case 11:
        __asm__ __volatile__(
        "sbi %[port], %[pin]\n\t"
        "nop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\t"
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
        "nop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\t"
        "cbi %[port], %[pin]\n\t"
        :
        : [port] "I" (_SFR_IO_ADDR(PORTB)), [pin] "I" (4)
        );
        break;
      case 14:
        __asm__ __volatile__(
        "sbi %[port], %[pin]\n\t"
        "nop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\t"
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
      case 16:
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

  volatile uint16_t width = steps / 4;

  cli();
  __asm__ __volatile__ (
    "movw r24, %[width]\n"   // load width into r24:r25
    "sbi %[port], %[bit]\n\t"
    "loop:\n"
        "sbiw r24, 1\n"
        "brne loop\n"
    "cbi %[port], %[bit]\n\t"
    :
    : [port] "I" (_SFR_IO_ADDR(PORTB)), [bit] "I" (4), [width] "r" (width)
    : "r24", "r25"
  );
  sei();

}

void old_generate_single_pulse_250ns(uint8_t steps) {
 
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
    : "r24", "r25"
  );

  // Restore Timer2 settings
  TCCR2A = oldTCCR2A;
  TCCR2B = oldTCCR2B;
  TCNT2  = oldTCNT2;
}