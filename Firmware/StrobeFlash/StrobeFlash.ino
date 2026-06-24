#include "pins.h"

constexpr uint16_t STROBE_ON_MS = 10;
constexpr uint16_t STROBE_PERIOD_MS = 500;

const uint8_t STROBE_PINS[] = { LEFT_STROBE, MIDDLE_STROBE, RIGHT_STROBE };

void setStrobes(bool on) {
    const uint8_t level = on ? HIGH : LOW;
    for (uint8_t pin : STROBE_PINS) {
        digitalWrite(pin, level);
    }
}

void setup() {
    for (uint8_t pin : STROBE_PINS) {
        pinMode(pin, OUTPUT);
    }
    setStrobes(false);
}

void loop() {
    setStrobes(true);
    delay(STROBE_ON_MS);
    setStrobes(false);
    delay(STROBE_PERIOD_MS - STROBE_ON_MS);
}
