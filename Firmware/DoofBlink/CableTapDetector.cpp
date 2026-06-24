#include "CableTapDetector.h"
#include "ChaseController.h"
#include "JoystickMapper.h"

void CableTapDetector::reset(Nunchuk &nunchuk) {
    armed_ = false;
    peakDelta_ = 0;
    peakCrossAxis_ = 0;
    baselineX_ = nunchuk.accelX();
    baselineY_ = nunchuk.accelY();
    baselineZ_ = nunchuk.accelZ();
}

void CableTapDetector::suppressAfterButton() {
    suppressUntilMs_ = millis() + STROBE_FADE_MS + ACCEL_SUPPRESS_AFTER_BUTTON_MS;
}

bool CableTapDetector::dominatesY(int16_t deltaY, int16_t deltaX, int16_t deltaZ) {
    const int16_t cross = max(abs(deltaX), abs(deltaZ));
    if (cross > CABLE_CROSS_AXIS_MAX) {
        return false;
    }
    if (deltaY < cross * CABLE_AXIS_DOMINANCE) {
        return false;
    }
    return true;
}

bool CableTapDetector::peakDominatesY() const {
    if (peakCrossAxis_ > CABLE_CROSS_AXIS_MAX) {
        return false;
    }
    if (peakDelta_ < peakCrossAxis_ * CABLE_AXIS_DOMINANCE) {
        return false;
    }
    return true;
}

void CableTapDetector::update(Nunchuk &nunchuk, ChaseController &chase,
                              uint8_t &latchedHue, uint8_t &latchedSat) {
    if (millis() < suppressUntilMs_ || nunchuk.buttonC() || nunchuk.buttonZ()) {
        armed_ = false;
        peakDelta_ = 0;
        peakCrossAxis_ = 0;
        return;
    }

    const int16_t ax = nunchuk.accelX();
    const int16_t ay = nunchuk.accelY();
    const int16_t az = nunchuk.accelZ();

    baselineX_ += (ax - baselineX_) * ACCEL_BASELINE_ALPHA;
    baselineY_ += (ay - baselineY_) * ACCEL_BASELINE_ALPHA;
    baselineZ_ += (az - baselineZ_) * ACCEL_BASELINE_ALPHA;

    const int16_t deltaX = ax - (int16_t)(baselineX_ + 0.5f);
    const int16_t deltaY = ay - (int16_t)(baselineY_ + 0.5f);
    const int16_t deltaZ = az - (int16_t)(baselineZ_ + 0.5f);

    if (deltaY <= 0) {
        armed_ = false;
        peakDelta_ = 0;
        peakCrossAxis_ = 0;
        return;
    }

    const int16_t crossNow = max(abs(deltaX), abs(deltaZ));

    if (!armed_) {
        if (deltaY >= CABLE_TAP_ARM_DELTA && dominatesY(deltaY, deltaX, deltaZ)) {
            armed_ = true;
            peakDelta_ = deltaY;
            peakCrossAxis_ = crossNow;
        }
        return;
    }

    if (deltaY > peakDelta_) {
        peakDelta_ = deltaY;
    }
    if (crossNow > peakCrossAxis_) {
        peakCrossAxis_ = crossNow;
    }

    if (deltaY > CABLE_TAP_RELEASE_DELTA) {
        return;
    }

    armed_ = false;
    const uint32_t now = millis();
    if (now - lastTapMs_ < CABLE_TAP_DEBOUNCE_MS || peakDelta_ < CABLE_TAP_PEAK_MIN) {
        return;
    }
    if (!peakDominatesY()) {
        return;
    }

    if (!JoystickMapper::atCenter(nunchuk.joyX(), nunchuk.joyY())) {
        return;
    }

    lastTapMs_ = now;
    chase.triggerBeat(nunchuk.joyX(), nunchuk.joyY(), now, latchedHue, latchedSat);
}
