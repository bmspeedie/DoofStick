#ifndef DOOF_LED_STRIPS_H
#define DOOF_LED_STRIPS_H

#include "DoofFastLED.h"
#include "config.h"
#include "pins.h"

class LedStrips {
public:
    void setup();
    void show(uint8_t brightness = 255);
    void fillBlack();
    CRGB *pixels();
    CRGB *fadeSnapshot();
    uint16_t count() const;

private:
    CRGB leds_[NUM_LEDS];
    CRGB fadeSnapshot_[NUM_LEDS];
    CLEDController *stripPd6_ = nullptr;
    CLEDController *stripPc6_ = nullptr;
    CLEDController *stripPd4_ = nullptr;
    CLEDController *stripPd7_ = nullptr;
};

#endif
