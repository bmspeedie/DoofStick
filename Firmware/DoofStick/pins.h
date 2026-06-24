#ifndef DOOF_PINS_H
#define DOOF_PINS_H

// DoofStick ATmega32U4 pin map (Arduino Leonardo numbering).
// Schematic net labels in comments.

// I2C (hardware Wire — Wii Nunchuk adapter)
#define I2C_SDA  2  // PD1
#define I2C_SCL  3  // PD0

// Strobes
#define RIGHT_STROBE   7   // PE6  — RIGHT_STROBE/3.5C
#define MIDDLE_STROBE  11  // PB7  — MIDDLE_STROBE/3.3C
#define LEFT_STROBE    30  // PD5  — LEFT_STROBE/3.1C (Leonardo core pin 30 / TX LED)

// WS2812 data (2.4 GHz / 2.5 GHz bands)
#define DATA1  12  // PD6 — DATA1/2.5A
#define DATA2   5  // PC6 — DATA2/2.5B
#define DATA3   9  // PB5 — DATA3/2.5B
#define DATA4   6  // PD7 — DATA4/2.5C
#define DATA5   8  // PB4 — DATA5/2.4A
#define DATA6   4  // PD4 — DATA6/2.4B
#define DATA7  10  // PB6 — DATA7/2.4B
#define DATA8  13  // PC7 — DATA8/2.4C

#endif
