#define FASTLED_ALLOW_INTERRUPTS 0
#include "globals.h"

void setup() {
    g_doofStick.setup();
}

void loop() {
    g_doofStick.loop();
}
