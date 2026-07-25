/**
 * Arduino LED Control Firmware
 */

// #include <avr/interrupt.h>
// #include <Adafruit_INA219.h>

#include "dac.h"
#include "generator.h"
#include "clkpulse.h"

#define STROBE_MODE_OFF 0
#define STROBE_MODE_GENERATOR 1
#define STROBE_MODE_CLKPULSE 2
#define STROBE_MODE_PCM 3
uint8_t strobe_mode;

struct pcm_param_t {
  size_t nsamples;
  uint8_t rate;
};

union {
  struct {
    uint16_t width;
    uint8_t on_level;
    uint8_t off_level;
  } clkpulse;

  pcm_param_t pcm;
} param;


generator_base* generator = 0;

static const size_t bufsize = 1024;
uint8_t databuffer[bufsize];

void status();
void start_strobe(uint32_t freq);
void stop_strobe();

void setup() {
  // Initialize serial communication
  Serial.begin(115200);

  // Set DAC pins as outputs
  DDRB |= 0x3F;  // D8–D13 as outputs
  DDRC |= 0x03;  // A0–A1 as outputs
  DDRD |= 0x04;  // D2 as output for ISR active indicator

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
      stop_strobe();
      set_dac(0xFF); 
      Serial.println("|OK|" + String(line));

    } else if (strncmp(line, "OFF", 3) == 0) {
      stop_strobe();
      set_dac(0x00);
      Serial.println("|OK|" + String(line));

    } else if (sscanf(line, "DIM:%d", &args[0]) == 1) {
      stop_strobe();
      set_dac(args[0]);
      Serial.println("|OK|" + String(line));

    } else if (sscanf(line, "STROBE:%d", &args[0]) == 1) {
      start_strobe(args[0]);
      // Serial.print(F("|OK|")); Serial.println(line);
      Serial.println("|OK|" + String(line));

    } else if (sscanf(line, "PCM:%d:%d", &args[0], &args[1]) == 2) {
      strobe_mode = STROBE_MODE_PCM;
      param.pcm.nsamples = args[1];
      param.pcm.rate = 16;  // hardcode to 1 sample per 16 clock cycles - 1 MHz 

      Serial.readBytes(databuffer, param.pcm.nsamples);

      Serial.print("pcm port=");
      Serial.print(args[0]);
      Serial.print(" rate="); Serial.print(param.pcm.rate);
      Serial.println("|OK|" + String(line));

    } else if (sscanf(line, "CLKPULSE:%d:%d:%d:%d", &args[0], &args[1], &args[2], &args[3]) == 4) {
      strobe_mode = STROBE_MODE_GENERATOR;
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

void start_strobe(uint32_t freq) {
  cli();

  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;

  // Prescalers in descending order; use the largest one where OCR1A fits in 16 bits.
  static const uint16_t prescalers[] = {1, 8, 64, 256, 1024};
  static const uint8_t  cs_bits[]    = {0x01, 0x02, 0x03, 0x04, 0x05};

  uint8_t  cs  = cs_bits[4];
  uint16_t ocr = 0;
  for (uint8_t i = 0; i < 5; i++) {
    uint32_t ticks = F_CPU / ((uint32_t)prescalers[i] * freq);
    Serial.print("period="); Serial.print(ticks*prescalers[i]);
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

void stop_strobe() {
  cli();
  // Disable Timer1 interrupts and stop the timer.
  TIMSK1 = 0;
  TCCR1B = 0;
  sei();
}

ISR(TIMER1_COMPA_vect) {
  PORTD |= (1 << PD2); // ISR active indicator -> HIGH

  switch (strobe_mode) {
    case(STROBE_MODE_GENERATOR):
      if (generator) {
        generator->generate();
      }
      break;

    case(STROBE_MODE_PCM):
      for(size_t i=0; i<param.pcm.nsamples;++i) {
        set_dac(databuffer[i]);
      }
      break;
  }

  // volatile uint16_t high_value = 255;
  // volatile uint16_t low_value = 0;

  // __asm__ __volatile__(
  //   "out %[port], %[h]\n\t"
  //   "nop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\t"
  //   "out %[port], %[l]\n\t"
  //   :
  //   : [port] "I" (_SFR_IO_ADDR(PORTB)), [h] "r" (high_value), [l] "r" (low_value)
  // );

  // __asm__ __volatile__(
  //   "nop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\t"
  // );


  PORTD &= ~(1 << PD2); // ISR active indicator -> LOW
}

