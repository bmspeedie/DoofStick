#include "DoofStickApp.h"

namespace {
alignas(DoofStickApp) uint8_t appStorage[sizeof(DoofStickApp)];
DoofStickApp *app = nullptr;
}

void setup() {
    // Construct after Arduino init() so Wire/USB are ready (global ctor runs too early).
    app = new (appStorage) DoofStickApp();
    app->setup();
}

void loop() {
    app->loop();
}
