// ZN426E-8 DAC test — Arduino Uno (ATmega328P)
//
// Pin mapping (8-bit DAC value):
//   bit 7 (MSB) → D13 = PB5
//   bit 6       → D12 = PB4
//   bit 5       → D11 = PB3
//   bit 4       → D10 = PB2
//   bit 3       → D9  = PB1
//   bit 2       → D8  = PB0
//   bit 1       → A1  = PC1
//   bit 0 (LSB) → A0  = PC0
//
// Readback: connect DAC OUT to A2 to verify ramp via Serial plotter.

void setDAC(uint8_t val) {
    PORTB = (PORTB & 0xC0) | (val >> 2);    // bits 7..2 → PB5..PB0
    PORTC = (PORTC & 0xFC) | (val & 0x03);  // bits 1..0 → PC1..PC0
}

// // Direct ADC read, bypasses analogRead() overhead.
// // channel: 0–5 maps to A0–A5 (A0/A1 are outputs here, use A2+ for readback)
// uint16_t readADC(uint8_t channel) {
//     ADMUX  = (1 << REFS0) | (channel & 0x07);
//     ADCSRA |= (1 << ADSC);
//     while (ADCSRA & (1 << ADSC));
//     return ADC;
// }

void setup() {
    Serial.begin(115200);
    DDRB |= 0x3F;  // D8–D13 as outputs
    DDRC |= 0x03;  // A0–A1 as outputs
    setDAC(0);
}

void loop() {
    static uint8_t val = 0;

    setDAC(val);
    // uint16_t feedback = readADC(2);  // A2

    Serial.println(val);
    // Serial.print('\t');
    // Serial.println(feedback);

    val++;  // wraps 255 → 0 automatically
    delay(1);
}
