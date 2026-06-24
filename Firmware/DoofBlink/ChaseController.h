#ifndef DOOF_CHASE_CONTROLLER_H
#define DOOF_CHASE_CONTROLLER_H

#include <FastLED.h>
#include <Arduino.h>
#include "config.h"

class BeatSync;
class LedStrips;
class StrobeController;

class ChaseController {
public:
    ChaseController(LedStrips &strips, BeatSync &beat, StrobeController &strobes);

    void start(uint8_t hue, uint8_t sat, uint16_t durationMs, bool fromStripEnd = true);
    void startCenter(uint8_t hue, uint8_t sat);
    void triggerRightDark(uint32_t now);
    void triggerBeat(uint8_t joyX, uint8_t joyY, uint32_t now, uint8_t &latchedHue, uint8_t &latchedSat);
    void startStripHoldFade(uint8_t hue, uint8_t sat);
    void startRainbowFade(bool &leftRainbowLit);

    void update(uint8_t joyX, uint8_t joyY);
    void updateCenter(uint8_t joyX, uint8_t joyY);
    void updateStripHoldFade();
    void updateLeftRainbow(uint8_t joyX, uint8_t joyY, uint8_t &latchedHue, uint8_t &latchedSat);
    void updateRightRainbow(uint8_t brightness);

    bool chaseActive() const;
    bool centerActive() const;
    bool rightDarkActive() const;
    bool stripFadeActive() const;
    void stopChase();
    void stopCenter();
    void stopRightDark();
    void stopStripFade();

    bool centerMode = false;
    bool dualMode = false;
    bool rainbowLinger = false;
    bool leftRainbowLit = false;

    void setCenterHueSat(uint8_t hue, uint8_t sat);
    void updateCenterHueSat(uint8_t joyX, uint8_t joyY);
    void beginCenterFromStick(uint8_t joyX, uint8_t joyY, uint8_t latchedHue, uint8_t latchedSat);

private:
    struct ChaseState {
        bool active = false;
        uint32_t startMs = 0;
        uint16_t durationMs = DEFAULT_CHASE_MS;
        uint8_t hue = 0;
        uint8_t sat = 255;
        bool fromStripEnd = false;
        bool endStrobeFired = false;
    };

    struct RightDarkChaseState {
        bool active = false;
        uint32_t startMs = 0;
    };

    struct CenterChaseState {
        bool active = false;
        uint32_t legStartMs = 0;
        bool expanding = true;
        uint8_t hue = 0;
        uint8_t sat = 255;
    };

    struct StripHoldFadeState {
        bool active = false;
        bool snapshot = false;
        uint32_t startMs = 0;
        uint8_t hue = 0;
        uint8_t sat = 255;
    };

    LedStrips &strips_;
    BeatSync &beat_;
    StrobeController &strobes_;

    ChaseState chase_;
    RightDarkChaseState rightDark_;
    CenterChaseState center_;
    StripHoldFadeState stripFade_;

    static uint8_t chaseBrightness(uint8_t offset, uint8_t fadeLeds, uint8_t coreLeds);
    static uint8_t centerCometBrightness(uint8_t offsetFromOuter, uint8_t fadeLeds, uint8_t coreLeds);
    static void chaseShape(uint8_t joyX, uint8_t joyY, uint8_t &fadeLeds, uint8_t &coreLeds);
    static void centerChaseShape(uint8_t &fadeLeds, uint8_t &coreLeds);

    void renderChase(float headPos, uint8_t hue, uint8_t sat, uint8_t joyX, uint8_t joyY);
    float rightChaseHeadPos() const;
    void renderRightRainbow(uint8_t brightness);
};

#endif
