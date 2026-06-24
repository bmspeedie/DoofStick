#include "ChaseController.h"
#include "BeatSync.h"
#include "JoystickMapper.h"
#include "LedStrips.h"
#include "StrobeController.h"

ChaseController::ChaseController(LedStrips &strips, BeatSync &beat, StrobeController &strobes)
    : strips_(strips), beat_(beat), strobes_(strobes) {}

bool ChaseController::chaseActive() const { return chase_.active; }
bool ChaseController::centerActive() const { return center_.active; }
bool ChaseController::rightDarkActive() const { return rightDark_.active; }
bool ChaseController::stripFadeActive() const { return stripFade_.active; }

void ChaseController::stopChase() { chase_.active = false; }
void ChaseController::stopCenter() { center_.active = false; }
void ChaseController::stopRightDark() { rightDark_.active = false; }
void ChaseController::stopStripFade() { stripFade_.active = false; }

void ChaseController::setCenterHueSat(uint8_t hue, uint8_t sat) {
    center_.hue = hue;
    center_.sat = sat;
}

void ChaseController::updateCenterHueSat(uint8_t joyX, uint8_t joyY) {
    JoystickMapper::zHoldColor(joyX, joyY, center_.hue, center_.sat);
}

void ChaseController::beginCenterFromStick(uint8_t joyX, uint8_t joyY,
                                           uint8_t latchedHue, uint8_t latchedSat) {
    center_.hue = latchedHue;
    center_.sat = latchedSat;
    JoystickMapper::zHoldColor(joyX, joyY, center_.hue, center_.sat);
    startCenter(center_.hue, center_.sat);
}

void ChaseController::start(uint8_t hue, uint8_t sat, uint16_t durationMs, bool fromStripEnd) {
    chase_.active = true;
    chase_.startMs = millis();
    chase_.durationMs = durationMs;
    chase_.hue = hue;
    chase_.sat = sat;
    chase_.fromStripEnd = fromStripEnd;
    chase_.endStrobeFired = false;
}

void ChaseController::startCenter(uint8_t hue, uint8_t sat) {
    center_.active = true;
    center_.legStartMs = millis();
    center_.expanding = true;
    center_.hue = hue;
    center_.sat = sat;
}

void ChaseController::triggerRightDark(uint32_t now) {
    beat_.updatePeriod(now);
    rightDark_.active = true;
    rightDark_.startMs = now;
}

void ChaseController::triggerBeat(uint8_t joyX, uint8_t joyY, uint32_t now,
                                  uint8_t &latchedHue, uint8_t &latchedSat) {
    rainbowLinger = false;
    if (JoystickMapper::inColorArc(joyX, joyY)) {
        latchedHue = JoystickMapper::hueFromStick(joyX, joyY);
        latchedSat = JoystickMapper::saturationFromStick(joyX, joyY);
    }
    beat_.updatePeriod(now);
    start(latchedHue, latchedSat, beat_.chaseDurationMs());
}

void ChaseController::startStripHoldFade(uint8_t hue, uint8_t sat) {
    stripFade_.active = true;
    stripFade_.snapshot = false;
    stripFade_.startMs = millis();
    stripFade_.hue = hue;
    stripFade_.sat = sat;
}

void ChaseController::startRainbowFade(bool &leftRainbowLit) {
    memcpy(strips_.fadeSnapshot(), strips_.pixels(), sizeof(CRGB) * NUM_LEDS);
    stripFade_.active = true;
    stripFade_.snapshot = true;
    stripFade_.startMs = millis();
    leftRainbowLit = false;
}

uint8_t ChaseController::chaseBrightness(uint8_t offset, uint8_t fadeLeds, uint8_t coreLeds) {
    if (offset < fadeLeds) {
        if (fadeLeds <= 1) {
            return 255;
        }
        return (uint8_t)((uint16_t)offset * 255 / (fadeLeds - 1));
    }

    if (offset < fadeLeds + coreLeds) {
        return 255;
    }

    const uint8_t leadOffset = offset - fadeLeds - coreLeds;
    if (fadeLeds <= 1) {
        return 0;
    }
    return 255 - (uint8_t)((uint16_t)leadOffset * 255 / (fadeLeds - 1));
}

uint8_t ChaseController::centerCometBrightness(uint8_t offsetFromOuter, uint8_t fadeLeds, uint8_t coreLeds) {
    const uint8_t width = fadeLeds * 2 + coreLeds;
    if (offsetFromOuter >= width) {
        return 0;
    }
    return chaseBrightness((uint8_t)(width - 1 - offsetFromOuter), fadeLeds, coreLeds);
}

void ChaseController::chaseShape(uint8_t joyX, uint8_t joyY, uint8_t &fadeLeds, uint8_t &coreLeds) {
    fadeLeds = CHASE_FADE_LEDS;
    coreLeds = CHASE_CORE_LEDS;

    if (!JoystickMapper::deflected(joyX, joyY)) {
        return;
    }

    fadeLeds = CHASE_FADE_LEDS * 3;
    coreLeds = CHASE_CORE_LEDS * 3;
}

void ChaseController::centerChaseShape(uint8_t &fadeLeds, uint8_t &coreLeds) {
    fadeLeds = CENTER_CHASE_FADE_LEDS;
    coreLeds = CENTER_CHASE_CORE_LEDS;
}

float ChaseController::rightChaseHeadPos() const {
    if (!rightDark_.active) {
        return -1000.0f;
    }

    const uint16_t durationMs = beat_.chaseDurationMs();
    const uint8_t fadeLeds = CHASE_FADE_LEDS * 2;
    const uint8_t coreLeds = CHASE_CORE_LEDS * 2;
    const uint8_t width = fadeLeds * 2 + coreLeds;
    const float speed = (float)(NUM_LEDS - 1) / (float)durationMs;
    const uint32_t elapsed = millis() - rightDark_.startMs;
    return (float)(NUM_LEDS - 1) - (float)elapsed * speed;
}

void ChaseController::renderRightRainbow(uint8_t brightness) {
    const uint8_t hueOffset = (uint8_t)(((millis() % RAINBOW_CYCLE_MS) * 256UL) / RAINBOW_CYCLE_MS);
    for (uint16_t i = 0; i < NUM_LEDS; i++) {
        strips_.pixels()[i] = CHSV(hueOffset + (uint8_t)((uint16_t)i * 256 / NUM_LEDS), 255, brightness);
    }
}

void ChaseController::updateLeftRainbow(uint8_t joyX, uint8_t joyY,
                                        uint8_t &latchedHue, uint8_t &latchedSat) {
    leftRainbowLit = true;
    uint8_t hue = latchedHue;
    uint8_t sat = latchedSat;
    JoystickMapper::zHoldColor(joyX, joyY, hue, sat);
    latchedHue = hue;
    latchedSat = sat;
    const uint8_t hueOffset = hue;
    for (uint16_t i = 0; i < NUM_LEDS; i++) {
        strips_.pixels()[i] = CHSV(hueOffset + (uint8_t)((uint16_t)i * 256 / NUM_LEDS), sat, 255);
    }
    strips_.show();
}

void ChaseController::updateRightRainbow(uint8_t brightness) {
    const uint8_t hueOffset = (uint8_t)(((millis() % RAINBOW_CYCLE_MS) * 256UL) / RAINBOW_CYCLE_MS);
    renderRightRainbow(brightness);

    if (!rightDark_.active) {
        strips_.show();
        return;
    }

    const uint8_t fadeLeds = CHASE_FADE_LEDS * 2;
    const uint8_t coreLeds = CHASE_CORE_LEDS * 2;
    const uint8_t width = fadeLeds * 2 + coreLeds;
    const float headPos = rightChaseHeadPos();

    if (headPos < -(float)width) {
        rightDark_.active = false;
        strips_.show();
        return;
    }

    const int head = (int)headPos;
    const int tailStart = head - (int)width + 1;

    for (int i = tailStart; i <= head; i++) {
        if (i < 0 || i >= NUM_LEDS) {
            continue;
        }
        const uint8_t offset = (uint8_t)(head - i);
        const uint8_t hole = chaseBrightness(offset, fadeLeds, coreLeds);
        strips_.pixels()[i] = CHSV(hueOffset + (uint8_t)((uint16_t)i * 256 / NUM_LEDS), 255,
            (uint8_t)((uint16_t)brightness * (255 - hole) / 255));
    }

    strips_.show();
}

void ChaseController::renderChase(float headPos, uint8_t hue, uint8_t sat, uint8_t joyX, uint8_t joyY) {
    uint8_t fadeLeds;
    uint8_t coreLeds;
    chaseShape(joyX, joyY, fadeLeds, coreLeds);
    const uint8_t width = fadeLeds * 2 + coreLeds;

    fill_solid(strips_.pixels(), strips_.count(), CRGB::Black);

    const int head = (int)headPos;
    const int tailStart = head - (int)width + 1;

    for (int i = tailStart; i <= head; i++) {
        if (i < 0 || i >= NUM_LEDS) {
            continue;
        }
        const uint8_t offset = (uint8_t)(head - i);
        const uint8_t brightness = chaseBrightness(offset, fadeLeds, coreLeds);
        strips_.pixels()[i] = CHSV(hue, sat, brightness);
    }
}

void ChaseController::update(uint8_t joyX, uint8_t joyY) {
    if (!chase_.active) {
        return;
    }

    const uint32_t elapsed = millis() - chase_.startMs;

    uint8_t fadeLeds;
    uint8_t coreLeds;
    chaseShape(joyX, joyY, fadeLeds, coreLeds);
    const uint8_t width = fadeLeds * 2 + coreLeds;

    const float speed = (float)(NUM_LEDS - 1) / (float)chase_.durationMs;
    float headPos;

    if (chase_.fromStripEnd) {
        headPos = (float)(NUM_LEDS - 1) - (float)elapsed * speed;
        const float firstBrightPos = headPos - (float)width + 1.0f + (float)fadeLeds;
        if (firstBrightPos <= 0.0f && !chase_.endStrobeFired && dualMode) {
            chase_.endStrobeFired = true;
            strobes_.startFade();
        }
        if (headPos < -(float)width) {
            if (dualMode) {
                start(chase_.hue, chase_.sat, chase_.durationMs, true);
            } else {
                chase_.active = false;
                strips_.fillBlack();
                strips_.show();
            }
            return;
        }
    } else {
        headPos = (float)elapsed * speed;
        if (headPos > (float)(NUM_LEDS - 1) + (float)width) {
            chase_.active = false;
            strips_.fillBlack();
            strips_.show();
            return;
        }
    }

    renderChase(headPos, chase_.hue, chase_.sat, joyX, joyY);
    strips_.show();
}

void ChaseController::updateCenter(uint8_t joyX, uint8_t joyY) {
    if (!center_.active) {
        return;
    }

    JoystickMapper::zHoldColor(joyX, joyY, center_.hue, center_.sat);

    uint8_t fadeLeds;
    uint8_t coreLeds;
    centerChaseShape(fadeLeds, coreLeds);
    const uint8_t width = fadeLeds * 2 + coreLeds;

    const float center = (float)(NUM_LEDS - 1) * 0.5f;
    const float maxReach = center * CENTER_CHASE_REACH_SCALE;
    const uint16_t legDuration = beat_.chaseDurationMs();
    uint32_t elapsed = millis() - center_.legStartMs;

    while (elapsed >= legDuration) {
        elapsed -= legDuration;
        center_.expanding = !center_.expanding;
        center_.legStartMs += legDuration;
    }

    const float t = (float)elapsed / (float)legDuration;
    const float reach = center_.expanding ? t * maxReach : (1.0f - t) * maxReach;
    const float rightHead = center + reach;
    const float leftHead = center - reach;

    fill_solid(strips_.pixels(), strips_.count(), CRGB::Black);
    for (uint16_t i = 0; i < NUM_LEDS; i++) {
        uint8_t brightness = 0;

        const float offRight = rightHead - (float)i;
        if (offRight >= 0.0f && offRight < (float)width) {
            brightness = max(brightness,
                centerCometBrightness((uint8_t)offRight, fadeLeds, coreLeds));
        }

        const float offLeft = (float)i - leftHead;
        if (offLeft >= 0.0f && offLeft < (float)width) {
            brightness = max(brightness,
                centerCometBrightness((uint8_t)offLeft, fadeLeds, coreLeds));
        }

        if (brightness > 0) {
            strips_.pixels()[i] = CHSV(center_.hue, center_.sat, brightness);
        }
    }
    strips_.show();
}

void ChaseController::updateStripHoldFade() {
    if (!stripFade_.active) {
        return;
    }

    const uint32_t elapsed = millis() - stripFade_.startMs;
    if (elapsed >= STROBE_FADE_MS) {
        stripFade_.active = false;
        strips_.fillBlack();
        strips_.show();
        return;
    }

    const uint8_t brightness = 255 - (uint8_t)((elapsed * 255UL) / STROBE_FADE_MS);
    if (stripFade_.snapshot) {
        memcpy(strips_.pixels(), strips_.fadeSnapshot(), sizeof(CRGB) * NUM_LEDS);
        strips_.show(brightness);
    } else {
        fill_solid(strips_.pixels(), strips_.count(),
            CHSV(stripFade_.hue, stripFade_.sat, brightness));
        strips_.show();
    }
}
