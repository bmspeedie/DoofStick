#include "JoystickMapper.h"

bool JoystickMapper::beyondDeadzone(int16_t dx, int16_t dy) {
    return max(abs(dx), abs(dy)) >= JOY_DEADZONE;
}

float JoystickMapper::angleFromUp(uint8_t joyX, uint8_t joyY) {
    const int16_t dx = (int16_t)joyX - JOY_CENTER;
    const int16_t dy = JOY_CENTER - (int16_t)joyY;
    float angle = atan2f((float)dx, (float)dy);
    if (angle < 0.0f) {
        angle += 2.0f * JOY_ARC_PI;
    }
    return angle;
}

bool JoystickMapper::inColorArc(uint8_t joyX, uint8_t joyY) {
    const int16_t dx = (int16_t)joyX - JOY_CENTER;
    const int16_t dy = (int16_t)joyY - JOY_CENTER;
    if (!beyondDeadzone(dx, dy)) {
        return false;
    }
    const float angle = angleFromUp(joyX, joyY);
    return angle <= JOY_RIGHT_ARC_LO || angle >= JOY_RIGHT_ARC_HI;
}

bool JoystickMapper::inRightArc(uint8_t joyX, uint8_t joyY) {
    const int16_t dx = (int16_t)joyX - JOY_CENTER;
    const int16_t dy = (int16_t)joyY - JOY_CENTER;
    if (!beyondDeadzone(dx, dy)) {
        return false;
    }
    const float angle = angleFromUp(joyX, joyY);
    return angle > JOY_RIGHT_ARC_LO && angle < JOY_RIGHT_ARC_HI;
}

bool JoystickMapper::inLeftArc(uint8_t joyX, uint8_t joyY) {
    const int16_t dx = (int16_t)joyX - JOY_CENTER;
    const int16_t dy = (int16_t)joyY - JOY_CENTER;
    if (!beyondDeadzone(dx, dy)) {
        return false;
    }
    const float angle = angleFromUp(joyX, joyY);
    return angle >= JOY_LEFT_ARC_LO && angle <= JOY_LEFT_ARC_HI;
}

bool JoystickMapper::atCenter(uint8_t joyX, uint8_t joyY) {
    return !inRightArc(joyX, joyY) && !inColorArc(joyX, joyY);
}

uint8_t JoystickMapper::hueOffsetFromLeftArc(uint8_t joyX, uint8_t joyY) {
    const float angle = angleFromUp(joyX, joyY);
    const float t = constrain((angle - JOY_LEFT_ARC_LO) / (2.0f * JOY_RIGHT_ARC_HALF), 0.0f, 1.0f);
    return (uint8_t)(t * 255.0f);
}

uint8_t JoystickMapper::hueFromStickWithDeadzone(uint8_t joyX, uint8_t joyY, int16_t deadzone) {
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

uint8_t JoystickMapper::hueFromStick(uint8_t joyX, uint8_t joyY) {
    return hueFromStickWithDeadzone(joyX, joyY, JOY_DEADZONE);
}

uint8_t JoystickMapper::saturationFromStickWithDeadzone(uint8_t joyX, uint8_t joyY, int16_t deadzone) {
    const int16_t dx = (int16_t)joyX - JOY_CENTER;
    const int16_t dy = (int16_t)joyY - JOY_CENTER;
    const int16_t deflection = max(abs(dx), abs(dy));
    return (uint8_t)constrain((deflection - deadzone) * 4, 64, 255);
}

uint8_t JoystickMapper::saturationFromStick(uint8_t joyX, uint8_t joyY) {
    return saturationFromStickWithDeadzone(joyX, joyY, JOY_DEADZONE);
}

void JoystickMapper::zHoldColor(uint8_t joyX, uint8_t joyY, uint8_t &hue, uint8_t &sat) {
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

bool JoystickMapper::deflected(uint8_t joyX, uint8_t joyY) {
    const int16_t dx = (int16_t)joyX - JOY_CENTER;
    const int16_t dy = (int16_t)joyY - JOY_CENTER;
    return max(abs(dx), abs(dy)) >= JOY_DEADZONE;
}
