/**
 * Arduino LED Control Firmware
 */

// #include <avr/interrupt.h>
// #include <Adafruit_INA219.h>

#include "dac.h"
#include "generator.h"
#include "clkpulse.h"

generator_base* generator = 0;

void status();
void startStrobe(uint32_t freq);
void stopStrobe();

void setup() {
  // Initialize serial communication
  Serial.begin(115200);

  // Set DAC pins as outputs
  DDRB |= 0x3F;  // D8–D13 as outputs
  DDRC |= 0x03;  // A0–A1 as outputs

  // Turn of DAC for LED
  set_dac(0);

  // Startup signal
  // Serial.println("READY");
}

void loop() {
  // Check for incoming serial data
  if (Serial.available() > 0) {
    
    char line[64];
    size_t n = Serial.readBytesUntil('\n', line, sizeof(line)-1);
    line[n] = '\0';   // terminate

    int args[3];

    if (strncmp(line, "ID", 2) == 0) {
      Serial.println(F("Arduino LED Controller|OK|ID"));

    } else if (strncmp(line, "STATUS", 6) == 0) {
      status();

    } else if (strncmp(line, "ON", 2) == 0) {
      stopStrobe();
      set_dac(0xFF); 
      Serial.println("|OK|" + String(line));

    } else if (strncmp(line, "OFF", 3) == 0) {
      stopStrobe();
      set_dac(0x00);
      Serial.println("|OK|" + String(line));

    } else if (sscanf(line, "DIM:%d", &args[0]) == 1) {
      set_dac(args[0]);
      Serial.println("|OK|" + String(line));

    } else if (sscanf(line, "STROBE:%d", &args[0]) == 1) {
      startStrobe(args[0]);
      Serial.println("|OK|" + String(line));

    } else if (sscanf(line, "CLKPULSE:%d:%d:%d:%d", &args[0], &args[1], &args[2], &args[3]) == 4) {
      delete generator;
      generator = new clk_pulse_generator(args[0], args[1], args[2], args[3]);
      Serial.println("|OK|" + String(line));
    } else {
      Serial.println("Unknown command|ERROR|" + String(line));
    }
  }
}

void status() {
  Serial.print("TCCR1B="); Serial.print(TCCR1B, HEX);
  Serial.print(" OCR1A="); Serial.print(OCR1A, HEX);
  Serial.println("|OK|STATUS");
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

