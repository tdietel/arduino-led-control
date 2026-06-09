#ifndef CLK_PULSE_H
#define CLK_PULSE_H

#include "generator.h"
#include "stdint.h"

class clk_pulse_generator : public generator_base {
  public:
    clk_pulse_generator(uint16_t duration, uint8_t high_value, uint8_t low_value); 
    virtual void generate();
  
  private:
    uint16_t duration; // in clock cycles (1 cycle = 62.5 ns at 16 MHz)
    volatile uint16_t loop_steps; // number of loop iterations for long pulses (>16clk)
    volatile uint8_t high_value;
    volatile uint8_t low_value;
};

#endif