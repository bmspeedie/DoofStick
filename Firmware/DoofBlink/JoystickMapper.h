#ifndef DOOF_JOYSTICK_MAPPER_H
#define DOOF_JOYSTICK_MAPPER_H

#include <Arduino.h>
#include "config.h"

class JoystickMapper {
public:
    static bool beyondDeadzone(int16_t dx, int16_t dy);
    static float angleFromUp(uint8_t joyX, uint8_t joyY);
    static bool inColorArc(uint8_t joyX, uint8_t joyY);
    static bool inRightArc(uint8_t joyX, uint8_t joyY);
    static bool inLeftArc(uint8_t joyX, uint8_t joyY);
    static bool atCenter(uint8_t joyX, uint8_t joyY);
    static bool deflected(uint8_t joyX, uint8_t joyY);
    static uint8_t hueOffsetFromLeftArc(uint8_t joyX, uint8_t joyY);
    static uint8_t hueFromStick(uint8_t joyX, uint8_t joyY);
    static uint8_t hueFromStickWithDeadzone(uint8_t joyX, uint8_t joyY, int16_t deadzone);
    static uint8_t saturationFromStick(uint8_t joyX, uint8_t joyY);
    static uint8_t saturationFromStickWithDeadzone(uint8_t joyX, uint8_t joyY, int16_t deadzone);
    static void zHoldColor(uint8_t joyX, uint8_t joyY, uint8_t &hue, uint8_t &sat);
};

#endif
