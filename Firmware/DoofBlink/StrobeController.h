#ifndef DOOF_STROBE_CONTROLLER_H
#define DOOF_STROBE_CONTROLLER_H

#include <Arduino.h>
#include "config.h"
#include "pins.h"

class LedStrips;

class StrobeController {
public:
    void setup();
    void on();
    void off();
    void startFade();
    void clearFade();
    void update(bool strobePressed, bool cHoldMode);
    void updateCStrobeHold(LedStrips &strips, uint8_t hue, uint8_t sat);

    bool pressed = false;
    bool cHoldMode = false;

private:
    struct FadeState {
        bool active = false;
        uint32_t startMs = 0;
    } fade_;

    static void writePin(uint8_t pin, uint8_t level);
};

#endif
