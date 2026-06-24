#ifndef DOOF_CABLE_TAP_DETECTOR_H
#define DOOF_CABLE_TAP_DETECTOR_H

#include <Arduino.h>
#include <NintendoExtensionCtrl.h>
#include "config.h"

class ChaseController;
class JoystickMapper;

class CableTapDetector {
public:
    void reset(Nunchuk &nunchuk);
    void suppressAfterButton();
    void update(Nunchuk &nunchuk, ChaseController &chase, uint8_t &latchedHue, uint8_t &latchedSat);

private:
    float baselineX_ = 512.0f;
    float baselineY_ = 512.0f;
    float baselineZ_ = 512.0f;
    bool armed_ = false;
    int16_t peakDelta_ = 0;
    int16_t peakCrossAxis_ = 0;
    uint32_t lastTapMs_ = 0;
    uint32_t suppressUntilMs_ = 0;

    static bool dominatesY(int16_t deltaY, int16_t deltaX, int16_t deltaZ);
    bool peakDominatesY() const;
};

#endif
