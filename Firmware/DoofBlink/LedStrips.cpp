#include "LedStrips.h"

void LedStrips::setup() {
    stripPd6_ = &FastLED.addLeds<WS2812, DATA1, GRB>(leds_, 0, NUM_LEDS);
    stripPc6_ = &FastLED.addLeds<WS2812, DATA2, GRB>(leds_, 0, NUM_LEDS);
    stripPd4_ = &FastLED.addLeds<WS2812, DATA6, GRB>(leds_, 0, NUM_LEDS);
    stripPd7_ = &FastLED.addLeds<WS2812, DATA4, GRB>(leds_, 0, NUM_LEDS);
    stripPd6_->setDither(0);
    stripPc6_->setDither(0);
    stripPd4_->setDither(0);
    stripPd7_->setDither(0);
    fillBlack();
    show();
}

void LedStrips::show(uint8_t brightness) {
    if (stripPd6_) {
        stripPd6_->showInternal(leds_, NUM_LEDS, brightness);
    }
    if (stripPc6_) {
        stripPc6_->showInternal(leds_, NUM_LEDS, brightness);
    }
    if (stripPd4_) {
        stripPd4_->showInternal(leds_, NUM_LEDS, brightness);
    }
    if (stripPd7_) {
        stripPd7_->showInternal(leds_, NUM_LEDS, brightness);
    }
}

void LedStrips::fillBlack() {
    fill_solid(leds_, NUM_LEDS, CRGB::Black);
}

CRGB *LedStrips::pixels() {
    return leds_;
}

CRGB *LedStrips::fadeSnapshot() {
    return fadeSnapshot_;
}

uint16_t LedStrips::count() const {
    return NUM_LEDS;
}
