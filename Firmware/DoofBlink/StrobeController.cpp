#include "StrobeController.h"
#include "LedStrips.h"
#include "pins.h"

namespace {
const uint8_t STROBE_PINS[] = {LEFT_STROBE, MIDDLE_STROBE, RIGHT_STROBE};
}

void StrobeController::setup() {
    // init() configures PD5 as the USB TX LED before setup(); reclaim it for LEFT_STROBE.
    pinMode(LEFT_STROBE, OUTPUT);
    digitalWrite(LEFT_STROBE, LOW);
    for (uint8_t pin : STROBE_PINS) {
        pinMode(pin, OUTPUT);
    }
    off();
}

void StrobeController::writePin(uint8_t pin, uint8_t level) {
    if (level == 0) {
        digitalWrite(pin, LOW);
        return;
    }
    if (digitalPinHasPWM(pin)) {
        analogWrite(pin, level);
    } else {
        digitalWrite(pin, HIGH);
    }
}

void StrobeController::on() {
    for (uint8_t pin : STROBE_PINS) {
        writePin(pin, 255);
    }
}

void StrobeController::off() {
    for (uint8_t pin : STROBE_PINS) {
        writePin(pin, 0);
    }
}

void StrobeController::startFade() {
    fade_.active = true;
    fade_.startMs = millis();
    on();
}

void StrobeController::clearFade() {
    fade_.active = false;
}

void StrobeController::update(bool strobePressed, bool cHoldMode) {
    if (strobePressed || cHoldMode || !fade_.active) {
        return;
    }

    const uint32_t elapsed = millis() - fade_.startMs;
    if (elapsed >= STROBE_FADE_MS) {
        off();
        fade_.active = false;
        return;
    }

    const uint8_t level = 255 - (uint8_t)((elapsed * 255UL) / STROBE_FADE_MS);
    for (uint8_t pin : STROBE_PINS) {
        writePin(pin, level);
    }
}

void StrobeController::updateCStrobeHold(LedStrips &strips, uint8_t hue, uint8_t sat) {
    const uint32_t phase = millis() % STRIP_FLASH_PERIOD_MS;
    if (phase < STRIP_FLASH_ON_MS) {
        fill_solid(strips.pixels(), strips.count(), CHSV(hue, sat, 255));
        on();
    } else {
        fill_solid(strips.pixels(), strips.count(), CRGB::Black);
        off();
    }
    strips.show();
}
