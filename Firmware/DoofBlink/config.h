#ifndef DOOF_CONFIG_H
#define DOOF_CONFIG_H

#include <Arduino.h>

constexpr uint16_t NUM_LEDS = 115;

constexpr uint16_t DEFAULT_CHASE_MS = 1000;
constexpr uint8_t BEAT_INTERVAL_HISTORY = 2;
constexpr uint16_t MIN_BEAT_MS = 250;
constexpr uint16_t MAX_BEAT_MS = 3000;
constexpr uint8_t CHASE_FADE_LEDS = 4;
constexpr uint8_t CHASE_CORE_LEDS = 8;
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

#endif
