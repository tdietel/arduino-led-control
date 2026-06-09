/**
 * Arduino LED Control Firmware with Timer-Based Pulse Generation
 * 
 * This sketch receives commands via serial connection to control LEDs.
 * Uses Timer1 interrupt to generate precise pulse patterns without blocking.
 * Compatible with the arduino-led-control Python module.
 */

#include <avr/interrupt.h>
#include <Adafruit_INA219.h>

#include "dac.h"
#include "generator.h"
#include "clkpulse.h"


// Configuration
int isConnected = 0;

generator_base* generator = 0;

// Variables
// String inputBuffer = "";
// const char COMMAND_DELIM = ':';
// const char LINE_ENDING = '\n';

// void setupTimer1();
// void interruptHandler();

void status();
void startStrobe(uint32_t freq);
void stopStrobe();
// void generate_single_pulse(uint16_t width);
// void generate_single_pulse_clk(uint16_t width);
// void generate_single_pulse_(uint8_t steps);

void send_announce_msg() {
  Serial.println("led_control.ino READY");
  // Serial.print("{\"status\":\"idle\",");
  // Serial.println("\"firmware_name\":\"led_control.ino\"}");
}

// Adafruit_INA219 ina219;

void setup() {
  // Initialize serial communication
  Serial.begin(115200);

  // Set DAC pins as outputs
  DDRB |= 0x3F;  // D8–D13 as outputs
  DDRC |= 0x03;  // A0–A1 as outputs

  // Turn of DAC for LED
  set_dac(0);

  // ina219.begin();

  // Startup signal
  Serial.println("READY");
  send_announce_msg();
}

void loop() {
  // Check for incoming serial data
  if (Serial.available() > 0) {
    
    char line[64];
    size_t n = Serial.readBytesUntil('\n', line, sizeof(line)-1);
    line[n] = '\0';   // terminate

    int args[3];

    if (strncmp(line, "CONNECT", 7) == 0) {
      isConnected = 1;
      Serial.println(String(line) + "|OK");

    } else if (strncmp(line, "DISCONNECT", 7) == 0) {
      isConnected = 0;
      Serial.println(String(line) + "|OK");

    } else if (strncmp(line, "STATUS", 6) == 0) {
      status();

    } else if (strncmp(line, "ON", 2) == 0) {
      stopStrobe();
      set_dac(0xFF); 
      Serial.println(String(line) + "|OK");

    } else if (strncmp(line, "OFF", 3) == 0) {
      stopStrobe();
      set_dac(0x00);
      Serial.println(String(line) + "|OK");

    } else if (sscanf(line, "DIM:%d", &args[0]) == 1) {
      set_dac(args[0]);
      Serial.println(String(line) + "|OK");

    } else if (sscanf(line, "STROBE:%d", &args[0]) == 1) {
      startStrobe(args[0]);
      Serial.println(String(line) + "|OK");

    } else if (sscanf(line, "CLKPULSE:%d:%d:%d", &args[0], &args[1], &args[2]) == 3) {
      delete generator;
      generator = new clk_pulse_generator(args[0], args[1], args[2]);
      Serial.println(String(line) + "|OK");


    } else {
      Serial.println("ERROR|Unknown command");
    }
  } else if (isConnected == 0) {
    // Send periodic status updates when not connected
    send_announce_msg();
    delay(2000);
  }
}

void status() {

  Serial.print("OK|");

  // Decode Timer1 prescaler from CS1[2:0] bits in TCCR1B
  uint8_t cs = TCCR1B & 0x07;
  int prescaler = 0;
  switch (cs) {
    case 1: prescaler = 1;    break;
    case 2: prescaler = 8;    break;
    case 3: prescaler = 64;   break;
    case 4: prescaler = 256;  break;
    case 5: prescaler = 1024; break;
    default: prescaler = 0;   break;  // timer stopped
  }

  if (prescaler > 0) {
    float freq = (float)F_CPU / ((float)prescaler * ((float)OCR1A + 1.0));
    Serial.print(",\"frequency_hz\":"); Serial.print(freq, 2);
    Serial.print(",\"prescaler\":"); Serial.print(prescaler);
    Serial.print(",\"ocr1a\":"); Serial.print(OCR1A);
  }

  Serial.println("}");
}

void startStrobe(uint32_t freq) {
  cli();

  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;

  // Prescalers in descending order; use the largest one where OCR1A fits in 16 bits.
  static const uint16_t prescalers[] = {1024, 256, 64, 8, 1};
  static const uint8_t  cs_bits[]    = {0x05, 0x04, 0x03, 0x02, 0x01};

  uint8_t  cs  = cs_bits[4];
  uint16_t ocr = 0;
  for (uint8_t i = 0; i < 5; i++) {
    uint32_t ticks = F_CPU / ((uint32_t)prescalers[i] * freq);
    if (ticks >= 1 && ticks <= 65536) {
      cs  = cs_bits[i];
      ocr = (uint16_t)(ticks - 1);
      break;
    }
  }

  OCR1A  = ocr;
  TCCR1B = (1 << WGM12) | cs;
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
  if (generator) {
    generator->generate();
  }
}

// void generate_single_pulse(uint16_t width) {

//   if (width == 0) {
//     return;
//   } else if (width < 16) {
//     generate_single_pulse_clk(width);
//   } else if (width < 256) {
//     generate_single_pulse_250ns(width);
//   } else {
//     // For longer pulses, use digitalWrite and delay
//     digitalWrite(12, HIGH);
//     delayMicroseconds(width/16);
//     digitalWrite(12, LOW);
//   }
// }

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
