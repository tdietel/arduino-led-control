#include "clkpulse.h"
#include <avr/io.h>
#include <avr/interrupt.h>


clk_pulse_generator::clk_pulse_generator(uint16_t duration, uint8_t high_value, uint8_t low_value) 
: duration(duration), loop_steps(duration/4), high_value(high_value), low_value(low_value)
{}


void clk_pulse_generator::generate() {
  cli();

  switch(duration){
    case 0:
      break;

    case 2:
      __asm__ __volatile__(
        "out %[port], %[h]\n\t" 
        "out %[port], %[l]\n\t"
        :
        : [port] "I" (_SFR_IO_ADDR(PORTB)), [h] "r" (high_value), [l] "r" (low_value)
      );
      break;

    case 3:
      __asm__ __volatile__(
        "out %[port], %[h]\n\t" 
        "nop\n\t"
        "out %[port], %[l]\n\t"
        :
        : [port] "I" (_SFR_IO_ADDR(PORTB)), [h] "r" (high_value), [l] "r" (low_value)
      );
      break;

    case 4:
      __asm__ __volatile__(
        "out %[port], %[h]\n\t" 
        "nop\n\tnop\n\t"
        "out %[port], %[l]\n\t"
        :
        : [port] "I" (_SFR_IO_ADDR(PORTB)), [h] "r" (high_value), [l] "r" (low_value)
      );
      break;
   
    case 5:
      __asm__ __volatile__(
        "out %[port], %[h]\n\t" 
        "nop\n\tnop\n\tnop\n\t"
        "out %[port], %[l]\n\t"
        :
        : [port] "I" (_SFR_IO_ADDR(PORTB)), [h] "r" (high_value), [l] "r" (low_value)
      );
      break;

      case 6:
        __asm__ __volatile__(
        "out %[port], %[h]\n\t"
        "nop\n\tnop\n\tnop\n\tnop\n\t"
        "out %[port], %[l]\n\t"
        :
        : [port] "I" (_SFR_IO_ADDR(PORTB)), [h] "r" (high_value), [l] "r" (low_value)
        );
        break;
      case 7:
        __asm__ __volatile__(
        "out %[port], %[h]\n\t"
        "nop\n\tnop\n\tnop\n\tnop\n\tnop\n\t"
        "out %[port], %[l]\n\t"
        :
        : [port] "I" (_SFR_IO_ADDR(PORTB)), [h] "r" (high_value), [l] "r" (low_value)
        );
        break;
      case 8:
        __asm__ __volatile__(
        "out %[port], %[h]\n\t"
        "nop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\t"
        "out %[port], %[l]\n\t"
        :
        : [port] "I" (_SFR_IO_ADDR(PORTB)), [h] "r" (high_value), [l] "r" (low_value)
        );
        break;
      case 9:
        __asm__ __volatile__(
        "out %[port], %[h]\n\t"
        "nop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\t"
        "out %[port], %[l]\n\t"
        :
        : [port] "I" (_SFR_IO_ADDR(PORTB)), [h] "r" (high_value), [l] "r" (low_value)
        );
        break;
      case 10:
        __asm__ __volatile__(
        "out %[port], %[h]\n\t"
        "nop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\t"
        "out %[port], %[l]\n\t"
        :
        : [port] "I" (_SFR_IO_ADDR(PORTB)), [h] "r" (high_value), [l] "r" (low_value)
        );
        break;
      case 11:
        __asm__ __volatile__(
        "out %[port], %[h]\n\t"
        "nop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\t"
        "out %[port], %[l]\n\t"
        :
        : [port] "I" (_SFR_IO_ADDR(PORTB)), [h] "r" (high_value), [l] "r" (low_value)
        );
        break;
      case 12:
        __asm__ __volatile__(
        "out %[port], %[h]\n\t"
        "nop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\t"
        "out %[port], %[l]\n\t"
        :
        : [port] "I" (_SFR_IO_ADDR(PORTB)), [h] "r" (high_value), [l] "r" (low_value)
        );
        break;
      case 13:
        __asm__ __volatile__(
        "out %[port], %[h]\n\t"
        "nop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\t"
        "out %[port], %[l]\n\t"
        :
        : [port] "I" (_SFR_IO_ADDR(PORTB)), [h] "r" (high_value), [l] "r" (low_value)
        );
        break;
      case 14:
        __asm__ __volatile__(
        "out %[port], %[h]\n\t"
        "nop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\t"
        "out %[port], %[l]\n\t"
        :
        : [port] "I" (_SFR_IO_ADDR(PORTB)), [h] "r" (high_value), [l] "r" (low_value)
        );
        break;
      case 15:
        __asm__ __volatile__(
        "out %[port], %[h]\n\t"
        "nop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\t"
        "out %[port], %[l]\n\t"
        :
        : [port] "I" (_SFR_IO_ADDR(PORTB)), [h] "r" (high_value), [l] "r" (low_value)
        );
        break;
      case 16:
        __asm__ __volatile__(
        "out %[port], %[h]\n\t"
        "nop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\t"
        "out %[port], %[l]\n\t"
        :
        : [port] "I" (_SFR_IO_ADDR(PORTB)), [h] "r" (high_value), [l] "r" (low_value)
        );
        break;
    default:
      __asm__ __volatile__ (
        "movw r24, %[width]\n"   // load width into r24:r25
        "out %[port], %[h]\n\t"
        "loop:\n"
            "sbiw r24, 1\n"
            "brne loop\n"
        "out %[port], %[l]\n\t"
        :
        : [port] "I" (_SFR_IO_ADDR(PORTB)), [h] "r" (high_value), [l] "r" (low_value), [width] "r" (loop_steps)
        : "r24", "r25"
      );
  }
  sei();

}
