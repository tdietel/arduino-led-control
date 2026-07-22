#ifndef DAC_H
#define DAC_H

inline void set_dac(uint8_t val) {
    PORTB = (PORTB & 0xC0) | (val >> 2);    // bits 7..2 → PB5..PB0
    PORTC = (PORTC & 0xFC) | (val & 0x03);  // bits 1..0 → PC1..PC0
}

inline uint8_t read_dac() {
    return ((PORTB & 0x3F) << 2) | (PORTC & 0x03);
}

#endif