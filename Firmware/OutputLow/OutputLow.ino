// Utility: drive all DoofStick outputs low (upload via ISP).
#include "pins.h"

const uint8_t OUTPUT_PINS[] = {
    RIGHT_STROBE, MIDDLE_STROBE, LEFT_STROBE,
    DATA1, DATA2, DATA3, DATA4, DATA5, DATA6, DATA7, DATA8,
};

void setup() {
    for (uint8_t pin : OUTPUT_PINS) {
        pinMode(pin, OUTPUT);
        digitalWrite(pin, LOW);
    }
}

void loop() {
}
