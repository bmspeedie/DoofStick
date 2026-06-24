#include "DoofStickApp.h"
#include "JoystickMapper.h"

DoofStickApp::DoofStickApp()
    : chase_(strips_, beat_, strobes_) {}

void DoofStickApp::updateColorLatch(uint8_t joyX, uint8_t joyY) {
    if (!JoystickMapper::inColorArc(joyX, joyY)) {
        return;
    }
    latchedHue_ = JoystickMapper::hueFromStick(joyX, joyY);
    latchedSat_ = JoystickMapper::saturationFromStick(joyX, joyY);
}

void DoofStickApp::updateButtons() {
    const bool buttonC = nunchuk_.buttonC();
    const bool buttonZ = nunchuk_.buttonZ();
    const bool bothHeld = buttonC && buttonZ;
    const uint32_t now = millis();

    if (bothHeld && !prevBothButtons_) {
        chase_.rainbowLinger = false;
        chase_.stopRightDark();
        chase_.dualMode = true;
        strobes_.pressed = false;
        strobes_.clearFade();
        chase_.centerMode = false;
        chase_.stopCenter();
        strobes_.off();
        cableTap_.reset(nunchuk_);
        updateColorLatch(nunchuk_.joyX(), nunchuk_.joyY());
        beat_.updatePeriod(now);
        chase_.start(latchedHue_, latchedSat_, beat_.chaseDurationMs(), true);
    }
    if (buttonC && buttonZ) {
        chase_.dualMode = true;
    } else if (!buttonC && !buttonZ) {
        chase_.dualMode = false;
    }
    prevBothButtons_ = bothHeld;

    if (chase_.dualMode) {
        prevButtonC_ = buttonC;
        prevButtonZ_ = buttonZ;
        return;
    }

    if (buttonC && !prevButtonC_ && !buttonZ) {
        buttonCHoldStartMs_ = now;
        strobes_.cHoldMode = false;
        strobes_.clearFade();
        strobes_.pressed = true;
        cableTap_.reset(nunchuk_);
        cableTap_.suppressAfterButton();
        strobes_.on();
    }

    if (buttonC && !buttonZ) {
        const uint32_t heldMs = now - buttonCHoldStartMs_;
        if (heldMs >= BUTTON_HOLD_MS) {
            if (!strobes_.cHoldMode) {
                strobes_.cHoldMode = true;
                strobes_.pressed = false;
                strobes_.clearFade();
                chase_.stopChase();
                chase_.rainbowLinger = false;
                updateColorLatch(nunchuk_.joyX(), nunchuk_.joyY());
            }
        }
    } else if (prevButtonC_ && !buttonZ) {
        const uint32_t heldMs = now - buttonCHoldStartMs_;
        if (strobes_.cHoldMode) {
            strobes_.cHoldMode = false;
            strobes_.off();
            strips_.fillBlack();
            strips_.show();
        } else if (heldMs < BUTTON_HOLD_MS) {
            strobes_.pressed = false;
            strobes_.startFade();
        }
    }

    if (strobes_.pressed) {
        strobes_.on();
    }

    if (buttonZ && !prevButtonZ_ && !buttonC) {
        buttonZHoldStartMs_ = now;
        chase_.centerMode = false;
        chase_.stopCenter();
        cableTap_.reset(nunchuk_);
        updateColorLatch(nunchuk_.joyX(), nunchuk_.joyY());
    }

    if (buttonZ && !buttonC) {
        if (millis() - buttonZHoldStartMs_ >= BUTTON_HOLD_MS) {
            chase_.rainbowLinger = false;
        }
    } else if (prevButtonZ_ && !buttonZ && !buttonC) {
        const uint32_t heldMs = now - buttonZHoldStartMs_;
        const uint8_t jx = nunchuk_.joyX();
        const uint8_t jy = nunchuk_.joyY();
        if (chase_.centerMode) {
            chase_.centerMode = false;
            chase_.stopCenter();
            strips_.fillBlack();
            strips_.show();
        } else if (prevLeftRainbowHold_ || chase_.leftRainbowLit) {
            chase_.startRainbowFade(chase_.leftRainbowLit);
            chase_.stopChase();
            chase_.centerMode = false;
            chase_.stopCenter();
        } else if (heldMs < BUTTON_HOLD_MS) {
            if (JoystickMapper::inRightArc(jx, jy)) {
                chase_.triggerRightDark(now);
            } else if (JoystickMapper::atCenter(jx, jy)) {
                chase_.triggerBeat(jx, jy, now, latchedHue_, latchedSat_);
            }
        }
    }

    prevButtonC_ = buttonC;
    prevButtonZ_ = buttonZ;
}

void DoofStickApp::renderFrame(uint8_t joyX, uint8_t joyY, bool buttonC, bool buttonZ, bool bothHeld) {
    const bool rightStickActive = JoystickMapper::inRightArc(joyX, joyY);
    const bool stickCenter = JoystickMapper::atCenter(joyX, joyY);
    const bool zLongHold = !chase_.dualMode && buttonZ && !buttonC
        && (millis() - buttonZHoldStartMs_ >= BUTTON_HOLD_MS);
    const bool leftRainbowHold = !chase_.dualMode && buttonZ && !buttonC
        && JoystickMapper::inColorArc(joyX, joyY) && !zLongHold;

    chase_.centerMode = zLongHold;

    if (!bothHeld) {
        if (prevRightStickActive_ && JoystickMapper::atCenter(joyX, joyY)) {
            chase_.rainbowLinger = true;
        }
        if (JoystickMapper::inColorArc(joyX, joyY)) {
            chase_.rainbowLinger = false;
        }
    }
    prevRightStickActive_ = rightStickActive;

    if (!chase_.centerMode && !strobes_.cHoldMode && !chase_.dualMode && !rightStickActive
            && !chase_.rainbowLinger && !leftRainbowHold && !chase_.stripFadeActive() && stickCenter) {
        cableTap_.update(nunchuk_, chase_, latchedHue_, latchedSat_);
    }

    if (strobes_.cHoldMode) {
        updateColorLatch(joyX, joyY);
        strobes_.updateCStrobeHold(strips_, latchedHue_, latchedSat_);
    } else if (chase_.dualMode) {
        chase_.update(joyX, joyY);
    } else if (zLongHold) {
        const uint8_t liveJoyX = nunchuk_.joyX();
        const uint8_t liveJoyY = nunchuk_.joyY();
        chase_.updateCenter(liveJoyX, liveJoyY);
    } else if (leftRainbowHold) {
        chase_.stopCenter();
        chase_.stopChase();
        chase_.stopStripFade();
        chase_.updateLeftRainbow(nunchuk_.joyX(), nunchuk_.joyY(), latchedHue_, latchedSat_);
    } else if (chase_.stripFadeActive() || prevLeftRainbowHold_) {
        if (!chase_.stripFadeActive()) {
            chase_.startRainbowFade(chase_.leftRainbowLit);
            chase_.stopChase();
            chase_.centerMode = false;
            chase_.stopCenter();
        }
        chase_.updateStripHoldFade();
    } else if (rightStickActive) {
        chase_.stopChase();
        chase_.updateRightRainbow(255);
    } else if (chase_.rainbowLinger && stickCenter) {
        chase_.stopChase();
        chase_.updateRightRainbow(RAINBOW_LINGER_BRIGHTNESS);
    } else if (chase_.chaseActive()) {
        chase_.update(joyX, joyY);
    }

    strobes_.update(strobes_.pressed);
    prevLeftRainbowHold_ = leftRainbowHold;
}

void DoofStickApp::setup() {
    strobes_.setup();
    strips_.setup();

    nunchuk_.begin();
    while (!nunchuk_.connect()) {
        delay(100);
    }
    cableTap_.reset(nunchuk_);
}

void DoofStickApp::loop() {
    if (!nunchuk_.update()) {
        nunchuk_.connect();
        delay(100);
        return;
    }

    if (nunchuk_.buttonZ()) {
        nunchuk_.update();
    }

    updateButtons();

    const uint8_t joyX = nunchuk_.joyX();
    const uint8_t joyY = nunchuk_.joyY();
    const bool buttonC = nunchuk_.buttonC();
    const bool buttonZ = nunchuk_.buttonZ();
    const bool bothHeld = buttonC && buttonZ;

    // Handle zLongHold center chase start (needs to run before renderFrame's mode checks)
    const bool zLongHold = !chase_.dualMode && buttonZ && !buttonC
        && (millis() - buttonZHoldStartMs_ >= BUTTON_HOLD_MS);
    if (zLongHold && !chase_.centerActive()) {
        chase_.stopChase();
        chase_.stopStripFade();
        chase_.stopRightDark();
        chase_.beginCenterFromStick(joyX, joyY, latchedHue_, latchedSat_);
    }

    renderFrame(joyX, joyY, buttonC, buttonZ, bothHeld);
}
