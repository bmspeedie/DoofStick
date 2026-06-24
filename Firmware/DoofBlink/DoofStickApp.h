#ifndef DOOF_STICK_APP_H
#define DOOF_STICK_APP_H

#include <NintendoExtensionCtrl.h>
#include <Arduino.h>
#include "BeatSync.h"
#include "CableTapDetector.h"
#include "ChaseController.h"
#include "LedStrips.h"
#include "StrobeController.h"
#include "config.h"

class DoofStickApp {
public:
    DoofStickApp();
    void setup();
    void loop();

private:
    Nunchuk nunchuk_;
    LedStrips strips_;
    StrobeController strobes_;
    BeatSync beat_;
    ChaseController chase_;
    CableTapDetector cableTap_;

    uint8_t latchedHue_ = 0;
    uint8_t latchedSat_ = 255;

    bool prevButtonC_ = false;
    bool prevButtonZ_ = false;
    bool prevBothButtons_ = false;
    bool prevRightStickActive_ = false;
    bool prevLeftRainbowHold_ = false;
    uint32_t buttonZHoldStartMs_ = 0;
    uint32_t buttonCHoldStartMs_ = 0;

    void updateColorLatch(uint8_t joyX, uint8_t joyY);
    void updateButtons();
    void renderFrame(uint8_t joyX, uint8_t joyY, bool buttonC, bool buttonZ, bool bothHeld);
};

#endif
