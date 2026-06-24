#define FASTLED_ALLOW_INTERRUPTS 0
#include <FastLED.h>
#include <NintendoExtensionCtrl.h>
#include "pins.h"

#define NUM_LEDS 115

constexpr uint16_t DEFAULT_CHASE_MS = 1000;
constexpr uint8_t BEAT_INTERVAL_HISTORY = 2;
constexpr uint16_t MIN_BEAT_MS = 250;
constexpr uint16_t MAX_BEAT_MS = 3000;
constexpr uint8_t CHASE_FADE_LEDS = 4;
constexpr uint8_t CHASE_CORE_LEDS = 8;
constexpr uint8_t CHASE_FADE_LEDS_MAX = 12;
constexpr uint8_t CHASE_CORE_LEDS_MAX = 56;
constexpr uint8_t CENTER_CHASE_FADE_LEDS = 4;
constexpr uint8_t CENTER_CHASE_CORE_LEDS = 4;
constexpr float CENTER_CHASE_REACH_SCALE = 1.0f;
constexpr uint16_t STROBE_FADE_MS = 200;
constexpr uint16_t BUTTON_HOLD_MS = 500;
constexpr uint16_t STRIP_FLASH_ON_MS = 20;
constexpr uint16_t STRIP_FLASH_OFF_MS = 100;
constexpr uint16_t STRIP_FLASH_PERIOD_MS = STRIP_FLASH_ON_MS + STRIP_FLASH_OFF_MS;
constexpr uint16_t RAINBOW_CYCLE_MS = 1000;
constexpr uint8_t RAINBOW_LINGER_BRIGHTNESS = 64;

constexpr int16_t JOY_CENTER = 128;
constexpr int16_t JOY_DEADZONE = 12;
constexpr float JOY_ARC_PI = 3.14159265f;
constexpr float JOY_RIGHT_ARC_HALF = JOY_ARC_PI / 4.0f;
constexpr float JOY_RIGHT_ARC_LO = JOY_ARC_PI / 2.0f - JOY_RIGHT_ARC_HALF;
constexpr float JOY_RIGHT_ARC_HI = JOY_ARC_PI / 2.0f + JOY_RIGHT_ARC_HALF;
constexpr float JOY_LEFT_ARC_LO = 3.0f * JOY_ARC_PI / 2.0f - JOY_RIGHT_ARC_HALF;
constexpr float JOY_LEFT_ARC_HI = 3.0f * JOY_ARC_PI / 2.0f + JOY_RIGHT_ARC_HALF;
constexpr float JOY_COLOR_ARC_SPAN = 1.5f * JOY_ARC_PI;

constexpr int16_t CABLE_TAP_ARM_DELTA = 320;
constexpr int16_t CABLE_TAP_RELEASE_DELTA = 120;
constexpr int16_t CABLE_TAP_PEAK_MIN = 320;
constexpr int16_t CABLE_CROSS_AXIS_MAX = 150;
constexpr uint8_t CABLE_AXIS_DOMINANCE = 2;
constexpr uint16_t CABLE_TAP_DEBOUNCE_MS = 200;
constexpr float ACCEL_BASELINE_ALPHA = 0.015f;
constexpr uint16_t ACCEL_SUPPRESS_AFTER_BUTTON_MS = 500;

const uint8_t STROBE_PINS[] = { LEFT_STROBE, MIDDLE_STROBE, RIGHT_STROBE };

CRGB leds[NUM_LEDS];
CRGB fadeSnapshot[NUM_LEDS];
Nunchuk nunchuk;

CLEDController *stripPd6 = nullptr;
CLEDController *stripPc6 = nullptr;
CLEDController *stripPd4 = nullptr;
CLEDController *stripPd7 = nullptr;

bool prevButtonC = false;
bool prevButtonZ = false;
bool prevBothButtons = false;
bool strobePressed = false;
bool cStrobeHoldMode = false;
bool centerChaseMode = false;
bool dualButtonMode = false;
bool rainbowLinger = false;
bool prevRightStickActive = false;
bool prevLeftRainbowHold = false;
bool leftRainbowLit = false;
uint32_t buttonZHoldStartMs = 0;
uint32_t buttonCHoldStartMs = 0;

uint8_t latchedHue = 0;
uint8_t latchedSat = 255;

float accelBaselineX = 512.0f;
float accelBaselineY = 512.0f;
float accelBaselineZ = 512.0f;
bool cableTapArmed = false;
int16_t cableTapPeakDelta = 0;
int16_t cableTapPeakCrossAxis = 0;
uint32_t lastCableTapMs = 0;
uint32_t accelSuppressUntilMs = 0;

float filteredBeatMs = DEFAULT_CHASE_MS;
uint32_t lastBeatMs = 0;
uint16_t beatIntervals[BEAT_INTERVAL_HISTORY] = {0, 0};
uint8_t beatIntervalCount = 0;
uint8_t beatIntervalIndex = 0;

struct ChaseState {
    bool active;
    uint32_t startMs;
    uint16_t durationMs;
    uint8_t hue;
    uint8_t sat;
    bool fromStripEnd;
    bool endStrobeFired;
} chase = {false, 0, DEFAULT_CHASE_MS, 0, 255, false, false};

struct StrobeState {
    bool active;
    uint32_t startMs;
} strobe = {false, 0};

struct RightDarkChaseState {
    bool active;
    uint32_t startMs;
} rightDarkChase = {false, 0};

struct CenterChaseState {
    bool active;
    uint32_t legStartMs;
    bool expanding;
    uint8_t hue;
    uint8_t sat;
} centerChase = {false, 0, true, 0, 255};

struct StripHoldFadeState {
    bool active;
    bool snapshot;
    uint32_t startMs;
    uint8_t hue;
    uint8_t sat;
} stripHoldFade = {false, false, 0, 0, 255};

bool stickBeyondDeadzone(int16_t dx, int16_t dy) {
    return max(abs(dx), abs(dy)) >= JOY_DEADZONE;
}

float stickAngleFromUp(uint8_t joyX, uint8_t joyY) {
    const int16_t dx = (int16_t)joyX - JOY_CENTER;
    const int16_t dy = JOY_CENTER - (int16_t)joyY;
    float angle = atan2f((float)dx, (float)dy);
    if (angle < 0.0f) {
        angle += 2.0f * JOY_ARC_PI;
    }
    return angle;
}

bool stickInColorArc(uint8_t joyX, uint8_t joyY) {
    const int16_t dx = (int16_t)joyX - JOY_CENTER;
    const int16_t dy = (int16_t)joyY - JOY_CENTER;
    if (!stickBeyondDeadzone(dx, dy)) {
        return false;
    }
    const float angle = stickAngleFromUp(joyX, joyY);
    return angle <= JOY_RIGHT_ARC_LO || angle >= JOY_RIGHT_ARC_HI;
}

bool stickInRightArc(uint8_t joyX, uint8_t joyY) {
    const int16_t dx = (int16_t)joyX - JOY_CENTER;
    const int16_t dy = (int16_t)joyY - JOY_CENTER;
    if (!stickBeyondDeadzone(dx, dy)) {
        return false;
    }
    const float angle = stickAngleFromUp(joyX, joyY);
    return angle > JOY_RIGHT_ARC_LO && angle < JOY_RIGHT_ARC_HI;
}

bool stickInLeftArc(uint8_t joyX, uint8_t joyY) {
    const int16_t dx = (int16_t)joyX - JOY_CENTER;
    const int16_t dy = (int16_t)joyY - JOY_CENTER;
    if (!stickBeyondDeadzone(dx, dy)) {
        return false;
    }
    const float angle = stickAngleFromUp(joyX, joyY);
    return angle >= JOY_LEFT_ARC_LO && angle <= JOY_LEFT_ARC_HI;
}

uint8_t hueOffsetFromLeftArc(uint8_t joyX, uint8_t joyY) {
    const float angle = stickAngleFromUp(joyX, joyY);
    const float t = constrain((angle - JOY_LEFT_ARC_LO) / (2.0f * JOY_RIGHT_ARC_HALF), 0.0f, 1.0f);
    return (uint8_t)(t * 255.0f);
}

uint8_t hueFromStickWithDeadzone(uint8_t joyX, uint8_t joyY, int16_t deadzone) {
    int16_t dx = (int16_t)joyX - JOY_CENTER;
    int16_t dy = JOY_CENTER - (int16_t)joyY;

    if (abs(dx) < deadzone) {
        dx = 0;
    }
    if (abs(dy) < deadzone) {
        dy = 0;
    }

    if (dx == 0 && dy == 0) {
        return 0;
    }

    float angle = atan2f((float)dx, (float)dy);
    if (angle < 0.0f) {
        angle += 2.0f * JOY_ARC_PI;
    }
    if (angle > JOY_RIGHT_ARC_LO && angle < JOY_RIGHT_ARC_HI) {
        return 0;
    }

    float t;
    if (angle >= JOY_RIGHT_ARC_HI) {
        t = (angle - JOY_RIGHT_ARC_HI) / JOY_COLOR_ARC_SPAN;
    } else {
        t = (angle + 2.0f * JOY_ARC_PI - JOY_RIGHT_ARC_HI) / JOY_COLOR_ARC_SPAN;
    }
    return (uint8_t)constrain(t * 255.0f, 0.0f, 255.0f);
}

uint8_t hueFromStick(uint8_t joyX, uint8_t joyY) {
    return hueFromStickWithDeadzone(joyX, joyY, JOY_DEADZONE);
}

uint8_t saturationFromStickWithDeadzone(uint8_t joyX, uint8_t joyY, int16_t deadzone) {
    const int16_t dx = (int16_t)joyX - JOY_CENTER;
    const int16_t dy = (int16_t)joyY - JOY_CENTER;
    const int16_t deflection = max(abs(dx), abs(dy));
    return (uint8_t)constrain((deflection - deadzone) * 4, 64, 255);
}

uint8_t saturationFromStick(uint8_t joyX, uint8_t joyY) {
    return saturationFromStickWithDeadzone(joyX, joyY, JOY_DEADZONE);
}

void updateZHoldColor(uint8_t joyX, uint8_t joyY, uint8_t &hue, uint8_t &sat) {
    const int16_t dx = (int16_t)joyX - JOY_CENTER;
    const int16_t dy = JOY_CENTER - (int16_t)joyY;

    float angle = atan2f((float)dx, (float)dy);
    if (angle < 0.0f) {
        angle += 2.0f * JOY_ARC_PI;
    }
    hue = (uint8_t)((angle / (2.0f * JOY_ARC_PI)) * 255.0f);

    const int16_t deflection = max(abs(dx), abs(dy));
    sat = (uint8_t)constrain(deflection * 2, 64, 255);
}

bool stickDeflected(uint8_t joyX, uint8_t joyY) {
    const int16_t dx = (int16_t)joyX - JOY_CENTER;
    const int16_t dy = (int16_t)joyY - JOY_CENTER;
    return max(abs(dx), abs(dy)) >= JOY_DEADZONE;
}

void showBothStrips(uint8_t brightness = 255) {
    if (stripPd6) {
        stripPd6->showInternal(leds, NUM_LEDS, brightness);
    }
    if (stripPc6) {
        stripPc6->showInternal(leds, NUM_LEDS, brightness);
    }
    if (stripPd4) {
        stripPd4->showInternal(leds, NUM_LEDS, brightness);
    }
    if (stripPd7) {
        stripPd7->showInternal(leds, NUM_LEDS, brightness);
    }
}

void updateColorLatch(uint8_t joyX, uint8_t joyY) {
    if (!stickInColorArc(joyX, joyY)) {
        return;
    }

    latchedHue = hueFromStick(joyX, joyY);
    latchedSat = saturationFromStick(joyX, joyY);
}

void updateCStrobeHold(uint8_t hue, uint8_t sat) {
    const uint32_t phase = millis() % STRIP_FLASH_PERIOD_MS;
    if (phase < STRIP_FLASH_ON_MS) {
        fill_solid(leds, NUM_LEDS, CHSV(hue, sat, 255));
        strobesOn();
    } else {
        fill_solid(leds, NUM_LEDS, CRGB::Black);
        strobesOff();
    }
    showBothStrips();
}

void startStripHoldFade(uint8_t hue, uint8_t sat) {
    stripHoldFade.active = true;
    stripHoldFade.snapshot = false;
    stripHoldFade.startMs = millis();
    stripHoldFade.hue = hue;
    stripHoldFade.sat = sat;
}

void startRainbowFade() {
    memcpy(fadeSnapshot, leds, sizeof(fadeSnapshot));
    stripHoldFade.active = true;
    stripHoldFade.snapshot = true;
    stripHoldFade.startMs = millis();
    leftRainbowLit = false;
}

void updateStripHoldFade() {
    if (!stripHoldFade.active) {
        return;
    }

    const uint32_t elapsed = millis() - stripHoldFade.startMs;
    if (elapsed >= STROBE_FADE_MS) {
        stripHoldFade.active = false;
        fill_solid(leds, NUM_LEDS, CRGB::Black);
        showBothStrips();
        return;
    }

    const uint8_t brightness = 255 - (uint8_t)((elapsed * 255UL) / STROBE_FADE_MS);
    if (stripHoldFade.snapshot) {
        memcpy(leds, fadeSnapshot, sizeof(fadeSnapshot));
        showBothStrips(brightness);
    } else {
        fill_solid(leds, NUM_LEDS, CHSV(stripHoldFade.hue, stripHoldFade.sat, brightness));
        showBothStrips();
    }
}

void writeStrobe(uint8_t pin, uint8_t level) {
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

void strobesOn() {
    for (uint8_t pin : STROBE_PINS) {
        digitalWrite(pin, HIGH);
    }
}

void strobesOff() {
    for (uint8_t pin : STROBE_PINS) {
        digitalWrite(pin, LOW);
    }
}

void startStrobes() {
    strobe.active = true;
    strobe.startMs = millis();
    strobesOn();
}

void updateStrobes() {
    if (strobePressed || !strobe.active) {
        return;
    }

    const uint32_t elapsed = millis() - strobe.startMs;
    if (elapsed >= STROBE_FADE_MS) {
        strobesOff();
        strobe.active = false;
        return;
    }

    const uint8_t level = 255 - (uint8_t)((elapsed * 255UL) / STROBE_FADE_MS);
    for (uint8_t pin : STROBE_PINS) {
        writeStrobe(pin, level);
    }
}

void updateBeatPeriod(uint32_t now) {
    if (lastBeatMs == 0) {
        lastBeatMs = now;
        return;
    }

    const uint32_t interval = now - lastBeatMs;
    lastBeatMs = now;

    if (interval < MIN_BEAT_MS || interval > MAX_BEAT_MS) {
        return;
    }

    beatIntervals[beatIntervalIndex] = (uint16_t)interval;
    beatIntervalIndex = (beatIntervalIndex + 1) % BEAT_INTERVAL_HISTORY;
    if (beatIntervalCount < BEAT_INTERVAL_HISTORY) {
        beatIntervalCount++;
    }

    uint32_t sum = 0;
    for (uint8_t i = 0; i < beatIntervalCount; i++) {
        sum += beatIntervals[i];
    }
    filteredBeatMs = (float)sum / beatIntervalCount;
}

uint16_t chaseDurationMs() {
    return (uint16_t)constrain((int32_t)(filteredBeatMs + 0.5f), MIN_BEAT_MS, MAX_BEAT_MS);
}

void chaseShape(uint8_t joyX, uint8_t joyY, uint8_t &fadeLeds, uint8_t &coreLeds) {
    fadeLeds = CHASE_FADE_LEDS;
    coreLeds = CHASE_CORE_LEDS;

    if (!stickDeflected(joyX, joyY)) {
        return;
    }

    fadeLeds = CHASE_FADE_LEDS * 3;
    coreLeds = CHASE_CORE_LEDS * 3;
}

void centerChaseShape(uint8_t joyX, uint8_t joyY, uint8_t &fadeLeds, uint8_t &coreLeds) {
    fadeLeds = CENTER_CHASE_FADE_LEDS;
    coreLeds = CENTER_CHASE_CORE_LEDS;
}

uint8_t centerCometBrightness(uint8_t offsetFromOuter, uint8_t fadeLeds, uint8_t coreLeds) {
    const uint8_t width = fadeLeds * 2 + coreLeds;
    if (offsetFromOuter >= width) {
        return 0;
    }
    return chaseBrightness((uint8_t)(width - 1 - offsetFromOuter), fadeLeds, coreLeds);
}

uint8_t chaseBrightness(uint8_t offset, uint8_t fadeLeds, uint8_t coreLeds) {
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

float rightChaseHeadPos() {
    if (!rightDarkChase.active) {
        return -1000.0f;
    }

    const uint16_t durationMs = chaseDurationMs();
    const uint8_t fadeLeds = CHASE_FADE_LEDS * 2;
    const uint8_t coreLeds = CHASE_CORE_LEDS * 2;
    const uint8_t width = fadeLeds * 2 + coreLeds;
    const float speed = (float)(NUM_LEDS - 1) / (float)durationMs;
    const uint32_t elapsed = millis() - rightDarkChase.startMs;
    return (float)(NUM_LEDS - 1) - (float)elapsed * speed;
}

bool stickAtCenter(uint8_t joyX, uint8_t joyY) {
    return !stickInRightArc(joyX, joyY) && !stickInColorArc(joyX, joyY);
}

void renderRightRainbow(uint8_t brightness) {
    const uint8_t hueOffset = (uint8_t)(((millis() % RAINBOW_CYCLE_MS) * 256UL) / RAINBOW_CYCLE_MS);
    for (uint16_t i = 0; i < NUM_LEDS; i++) {
        leds[i] = CHSV(hueOffset + (uint8_t)((uint16_t)i * 256 / NUM_LEDS), 255, brightness);
    }
}

void updateLeftRainbowMode(uint8_t joyX, uint8_t joyY) {
    leftRainbowLit = true;
    uint8_t hue = latchedHue;
    uint8_t sat = latchedSat;
    updateZHoldColor(joyX, joyY, hue, sat);
    latchedHue = hue;
    latchedSat = sat;
    const uint8_t hueOffset = hue;
    for (uint16_t i = 0; i < NUM_LEDS; i++) {
        leds[i] = CHSV(hueOffset + (uint8_t)((uint16_t)i * 256 / NUM_LEDS), sat, 255);
    }
    showBothStrips();
}

void updateRightRainbowMode(uint8_t brightness) {
    const uint8_t hueOffset = (uint8_t)(((millis() % RAINBOW_CYCLE_MS) * 256UL) / RAINBOW_CYCLE_MS);
    renderRightRainbow(brightness);

    if (!rightDarkChase.active) {
        showBothStrips();
        return;
    }

    const uint8_t fadeLeds = CHASE_FADE_LEDS * 2;
    const uint8_t coreLeds = CHASE_CORE_LEDS * 2;
    const uint8_t width = fadeLeds * 2 + coreLeds;
    const float headPos = rightChaseHeadPos();

    if (headPos < -(float)width) {
        rightDarkChase.active = false;
        showBothStrips();
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
        leds[i] = CHSV(hueOffset + (uint8_t)((uint16_t)i * 256 / NUM_LEDS), 255,
            (uint8_t)((uint16_t)brightness * (255 - hole) / 255));
    }

    showBothStrips();
}

void triggerRightDarkChase(uint32_t now) {
    updateBeatPeriod(now);
    rightDarkChase.active = true;
    rightDarkChase.startMs = now;
}

void renderChase(float headPos, uint8_t hue, uint8_t sat, uint8_t joyX, uint8_t joyY) {
    uint8_t fadeLeds;
    uint8_t coreLeds;
    chaseShape(joyX, joyY, fadeLeds, coreLeds);
    const uint8_t width = fadeLeds * 2 + coreLeds;

    fill_solid(leds, NUM_LEDS, CRGB::Black);

    const int head = (int)headPos;
    const int tailStart = head - (int)width + 1;

    for (int i = tailStart; i <= head; i++) {
        if (i < 0 || i >= NUM_LEDS) {
            continue;
        }
        const uint8_t offset = (uint8_t)(head - i);
        const uint8_t brightness = chaseBrightness(offset, fadeLeds, coreLeds);
        leds[i] = CHSV(hue, sat, brightness);
    }
}

void startChase(uint8_t hue, uint8_t sat, uint16_t durationMs, bool fromStripEnd = true) {
    chase.active = true;
    chase.startMs = millis();
    chase.durationMs = durationMs;
    chase.hue = hue;
    chase.sat = sat;
    chase.fromStripEnd = fromStripEnd;
    chase.endStrobeFired = false;
}

void startCenterChase(uint8_t hue, uint8_t sat) {
    centerChase.active = true;
    centerChase.legStartMs = millis();
    centerChase.expanding = true;
    centerChase.hue = hue;
    centerChase.sat = sat;
}

void updateCenterChase(uint8_t joyX, uint8_t joyY) {
    if (!centerChase.active) {
        return;
    }

    updateZHoldColor(joyX, joyY, centerChase.hue, centerChase.sat);

    uint8_t fadeLeds;
    uint8_t coreLeds;
    centerChaseShape(joyX, joyY, fadeLeds, coreLeds);
    const uint8_t width = fadeLeds * 2 + coreLeds;

    const float center = (float)(NUM_LEDS - 1) * 0.5f;
    const float maxReach = center * CENTER_CHASE_REACH_SCALE;
    const uint16_t legDuration = chaseDurationMs();
    uint32_t elapsed = millis() - centerChase.legStartMs;

    while (elapsed >= legDuration) {
        elapsed -= legDuration;
        centerChase.expanding = !centerChase.expanding;
        centerChase.legStartMs += legDuration;
    }

    const float t = (float)elapsed / (float)legDuration;
    const float reach = centerChase.expanding ? t * maxReach : (1.0f - t) * maxReach;
    const float rightHead = center + reach;
    const float leftHead = center - reach;

    fill_solid(leds, NUM_LEDS, CRGB::Black);
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
            leds[i] = CHSV(centerChase.hue, centerChase.sat, brightness);
        }
    }
    showBothStrips();
}

void updateChase() {
    if (!chase.active) {
        return;
    }

    const uint32_t elapsed = millis() - chase.startMs;
    const uint8_t joyX = nunchuk.joyX();
    const uint8_t joyY = nunchuk.joyY();

    uint8_t fadeLeds;
    uint8_t coreLeds;
    chaseShape(joyX, joyY, fadeLeds, coreLeds);
    const uint8_t width = fadeLeds * 2 + coreLeds;

    const float speed = (float)(NUM_LEDS - 1) / (float)chase.durationMs;
    float headPos;

    if (chase.fromStripEnd) {
        headPos = (float)(NUM_LEDS - 1) - (float)elapsed * speed;
        const float firstBrightPos = headPos - (float)width + 1.0f + (float)fadeLeds;
        if (firstBrightPos <= 0.0f && !chase.endStrobeFired && dualButtonMode) {
            chase.endStrobeFired = true;
            startStrobes();
        }
        if (headPos < -(float)width) {
            if (dualButtonMode) {
                startChase(chase.hue, chase.sat, chase.durationMs, true);
            } else {
                chase.active = false;
                fill_solid(leds, NUM_LEDS, CRGB::Black);
                showBothStrips();
            }
            return;
        }
    } else {
        headPos = (float)elapsed * speed;
        if (headPos > (float)(NUM_LEDS - 1) + (float)width) {
            chase.active = false;
            fill_solid(leds, NUM_LEDS, CRGB::Black);
            showBothStrips();
            return;
        }
    }

    renderChase(headPos, chase.hue, chase.sat, joyX, joyY);
    showBothStrips();
}

void triggerBeat(uint8_t joyX, uint8_t joyY, uint32_t now) {
    rainbowLinger = false;
    updateColorLatch(joyX, joyY);
    updateBeatPeriod(now);
    startChase(latchedHue, latchedSat, chaseDurationMs());
}

void resetCableTapDetector() {
    cableTapArmed = false;
    cableTapPeakDelta = 0;
    cableTapPeakCrossAxis = 0;
    accelBaselineX = nunchuk.accelX();
    accelBaselineY = nunchuk.accelY();
    accelBaselineZ = nunchuk.accelZ();
}

bool cableTapDominatesY(int16_t deltaY, int16_t deltaX, int16_t deltaZ) {
    const int16_t cross = max(abs(deltaX), abs(deltaZ));
    if (cross > CABLE_CROSS_AXIS_MAX) {
        return false;
    }
    if (deltaY < cross * CABLE_AXIS_DOMINANCE) {
        return false;
    }
    return true;
}

bool cableTapPeakDominatesY() {
    if (cableTapPeakCrossAxis > CABLE_CROSS_AXIS_MAX) {
        return false;
    }
    if (cableTapPeakDelta < cableTapPeakCrossAxis * CABLE_AXIS_DOMINANCE) {
        return false;
    }
    return true;
}

void suppressAccelAfterButton() {
    accelSuppressUntilMs = millis() + STROBE_FADE_MS + ACCEL_SUPPRESS_AFTER_BUTTON_MS;
}

void updateCableTapDetector() {
    if (millis() < accelSuppressUntilMs || nunchuk.buttonC() || nunchuk.buttonZ()) {
        cableTapArmed = false;
        cableTapPeakDelta = 0;
        cableTapPeakCrossAxis = 0;
        return;
    }

    const int16_t ax = nunchuk.accelX();
    const int16_t ay = nunchuk.accelY();
    const int16_t az = nunchuk.accelZ();

    accelBaselineX += (ax - accelBaselineX) * ACCEL_BASELINE_ALPHA;
    accelBaselineY += (ay - accelBaselineY) * ACCEL_BASELINE_ALPHA;
    accelBaselineZ += (az - accelBaselineZ) * ACCEL_BASELINE_ALPHA;

    const int16_t deltaX = ax - (int16_t)(accelBaselineX + 0.5f);
    const int16_t deltaY = ay - (int16_t)(accelBaselineY + 0.5f);
    const int16_t deltaZ = az - (int16_t)(accelBaselineZ + 0.5f);

    if (deltaY <= 0) {
        cableTapArmed = false;
        cableTapPeakDelta = 0;
        cableTapPeakCrossAxis = 0;
        return;
    }

    const int16_t crossNow = max(abs(deltaX), abs(deltaZ));

    if (!cableTapArmed) {
        if (deltaY >= CABLE_TAP_ARM_DELTA && cableTapDominatesY(deltaY, deltaX, deltaZ)) {
            cableTapArmed = true;
            cableTapPeakDelta = deltaY;
            cableTapPeakCrossAxis = crossNow;
        }
        return;
    }

    if (deltaY > cableTapPeakDelta) {
        cableTapPeakDelta = deltaY;
    }
    if (crossNow > cableTapPeakCrossAxis) {
        cableTapPeakCrossAxis = crossNow;
    }

    if (deltaY > CABLE_TAP_RELEASE_DELTA) {
        return;
    }

    cableTapArmed = false;
    const uint32_t now = millis();
    if (now - lastCableTapMs < CABLE_TAP_DEBOUNCE_MS || cableTapPeakDelta < CABLE_TAP_PEAK_MIN) {
        return;
    }
    if (!cableTapPeakDominatesY()) {
        return;
    }

    if (!stickAtCenter(nunchuk.joyX(), nunchuk.joyY())) {
        return;
    }

    lastCableTapMs = now;
    triggerBeat(nunchuk.joyX(), nunchuk.joyY(), now);
}

void updateButtons() {
    const bool buttonC = nunchuk.buttonC();
    const bool buttonZ = nunchuk.buttonZ();
    const bool bothHeld = buttonC && buttonZ;
    const uint32_t now = millis();

    if (bothHeld && !prevBothButtons) {
        rainbowLinger = false;
        rightDarkChase.active = false;
        dualButtonMode = true;
        strobePressed = false;
        strobe.active = false;
        centerChaseMode = false;
        centerChase.active = false;
        strobesOff();
        resetCableTapDetector();
        updateColorLatch(nunchuk.joyX(), nunchuk.joyY());
        updateBeatPeriod(now);
        startChase(latchedHue, latchedSat, chaseDurationMs(), true);
    }
    if (buttonC && buttonZ) {
        dualButtonMode = true;
    } else if (!buttonC && !buttonZ) {
        dualButtonMode = false;
    }
    prevBothButtons = bothHeld;

    if (dualButtonMode) {
        prevButtonC = buttonC;
        prevButtonZ = buttonZ;
        return;
    }

    // C (big): short hold = solid on + fade on release; hold >= 500 ms = strobe flash.
    if (buttonC && !prevButtonC && !buttonZ) {
        buttonCHoldStartMs = now;
        cStrobeHoldMode = false;
        strobe.active = false;
        strobePressed = true;
        resetCableTapDetector();
        suppressAccelAfterButton();
        strobesOn();
    }

    if (buttonC && !buttonZ) {
        const uint32_t heldMs = now - buttonCHoldStartMs;
        if (heldMs >= BUTTON_HOLD_MS) {
            if (!cStrobeHoldMode) {
                cStrobeHoldMode = true;
                strobePressed = false;
                strobe.active = false;
                chase.active = false;
                rainbowLinger = false;
                updateColorLatch(nunchuk.joyX(), nunchuk.joyY());
            }
        }
    } else if (prevButtonC && !buttonZ) {
        const uint32_t heldMs = now - buttonCHoldStartMs;
        if (cStrobeHoldMode) {
            cStrobeHoldMode = false;
            strobesOff();
            fill_solid(leds, NUM_LEDS, CRGB::Black);
            showBothStrips();
        } else if (heldMs < BUTTON_HOLD_MS) {
            strobePressed = false;
            startStrobes();
        }
    }

    if (strobePressed) {
        strobesOn();
    }

    // Z (little): center short tap = chase; right short tap = dark chase; hold >= 500 ms = center bounce chase.
    if (buttonZ && !prevButtonZ && !buttonC) {
        buttonZHoldStartMs = now;
        centerChaseMode = false;
        centerChase.active = false;
        resetCableTapDetector();
        updateColorLatch(nunchuk.joyX(), nunchuk.joyY());
    }

    if (buttonZ && !buttonC) {
        if (millis() - buttonZHoldStartMs >= BUTTON_HOLD_MS) {
            rainbowLinger = false;
        }
    } else if (prevButtonZ && !buttonZ && !buttonC) {
        const uint32_t heldMs = now - buttonZHoldStartMs;
        const uint8_t jx = nunchuk.joyX();
        const uint8_t jy = nunchuk.joyY();
        if (centerChaseMode) {
            centerChaseMode = false;
            centerChase.active = false;
            fill_solid(leds, NUM_LEDS, CRGB::Black);
            showBothStrips();
        } else if (prevLeftRainbowHold || leftRainbowLit) {
            startRainbowFade();
            chase.active = false;
            centerChaseMode = false;
            centerChase.active = false;
        } else if (heldMs < BUTTON_HOLD_MS) {
            if (stickInRightArc(jx, jy)) {
                triggerRightDarkChase(now);
            } else if (stickAtCenter(jx, jy)) {
                triggerBeat(jx, jy, now);
            }
        }
    }

    prevButtonC = buttonC;
    prevButtonZ = buttonZ;
}

void setup() {
    for (uint8_t pin : STROBE_PINS) {
        pinMode(pin, OUTPUT);
    }
    strobesOff();

    stripPd6 = &FastLED.addLeds<WS2812, DATA1, GRB>(leds, 0, NUM_LEDS);
    stripPc6 = &FastLED.addLeds<WS2812, DATA2, GRB>(leds, 0, NUM_LEDS);
    stripPd4 = &FastLED.addLeds<WS2812, DATA6, GRB>(leds, 0, NUM_LEDS);
    stripPd7 = &FastLED.addLeds<WS2812, DATA4, GRB>(leds, 0, NUM_LEDS);
    stripPd6->setDither(0);
    stripPc6->setDither(0);
    stripPd4->setDither(0);
    stripPd7->setDither(0);
    fill_solid(leds, NUM_LEDS, CRGB::Black);
    showBothStrips();

    nunchuk.begin();
    while (!nunchuk.connect()) {
        delay(100);
    }
    accelBaselineX = nunchuk.accelX();
    accelBaselineY = nunchuk.accelY();
    accelBaselineZ = nunchuk.accelZ();
}

void loop() {
    if (!nunchuk.update()) {
        nunchuk.connect();
        delay(100);
        return;
    }

    if (nunchuk.buttonZ()) {
        nunchuk.update();
    }

    updateButtons();

    const uint8_t joyX = nunchuk.joyX();
    const uint8_t joyY = nunchuk.joyY();
    const bool buttonC = nunchuk.buttonC();
    const bool buttonZ = nunchuk.buttonZ();
    const bool bothHeld = buttonC && buttonZ;
    const bool rightStickActive = stickInRightArc(joyX, joyY);
    const bool stickCenter = stickAtCenter(joyX, joyY);
    const bool zLongHold = !dualButtonMode && buttonZ && !buttonC
        && (millis() - buttonZHoldStartMs >= BUTTON_HOLD_MS);
    const bool leftRainbowHold = !dualButtonMode && buttonZ && !buttonC
        && stickInColorArc(joyX, joyY) && !zLongHold;

    if (zLongHold && !centerChase.active) {
        chase.active = false;
        stripHoldFade.active = false;
        rightDarkChase.active = false;
        centerChase.hue = latchedHue;
        centerChase.sat = latchedSat;
        updateZHoldColor(joyX, joyY, centerChase.hue, centerChase.sat);
        startCenterChase(centerChase.hue, centerChase.sat);
    }
    centerChaseMode = zLongHold;

    if (!bothHeld) {
        if (prevRightStickActive && stickAtCenter(joyX, joyY)) {
            rainbowLinger = true;
        }
        if (stickInColorArc(joyX, joyY)) {
            rainbowLinger = false;
        }
    }
    prevRightStickActive = rightStickActive;

    if (!centerChaseMode && !cStrobeHoldMode && !dualButtonMode && !rightStickActive
            && !rainbowLinger && !leftRainbowHold && !stripHoldFade.active && stickCenter) {
        updateCableTapDetector();
    }
    if (cStrobeHoldMode) {
        updateColorLatch(joyX, joyY);
        updateCStrobeHold(latchedHue, latchedSat);
    } else if (dualButtonMode) {
        updateChase();
    } else if (zLongHold) {
        const uint8_t liveJoyX = nunchuk.joyX();
        const uint8_t liveJoyY = nunchuk.joyY();
        updateCenterChase(liveJoyX, liveJoyY);
    } else if (leftRainbowHold) {
        centerChase.active = false;
        chase.active = false;
        stripHoldFade.active = false;
        updateLeftRainbowMode(nunchuk.joyX(), nunchuk.joyY());
    } else if (stripHoldFade.active || prevLeftRainbowHold) {
        if (!stripHoldFade.active) {
            startRainbowFade();
            chase.active = false;
            centerChaseMode = false;
            centerChase.active = false;
        }
        updateStripHoldFade();
    } else if (rightStickActive) {
        chase.active = false;
        updateRightRainbowMode(255);
    } else if (rainbowLinger && stickCenter) {
        chase.active = false;
        updateRightRainbowMode(RAINBOW_LINGER_BRIGHTNESS);
    } else if (chase.active) {
        updateChase();
    }

    updateStrobes();

    prevLeftRainbowHold = leftRainbowHold;
}
