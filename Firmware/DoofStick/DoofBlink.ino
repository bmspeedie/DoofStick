#include "FastLED.h"
#include "fl/sketch_macros.h"
#include <NintendoExtensionCtrl.h>
#include "pins.h"

#define NUM_LEDS 60
#define NUM_STRIPS 8

// Z-axis tap → one full cascade along the tape (0 .. NUM_LEDS-1).
#define CASCADE_TAIL 12
#define CASCADE_MIN_MS 70
#define CASCADE_MAX_MS 550
#define TAP_ARM_DELTA 55
#define TAP_RELEASE_DELTA 28
#define TAP_DEBOUNCE_MS 90
#define TAP_PEAK_MIN 55
#define TAP_PEAK_MAX 280
#define BASELINE_ALPHA 0.06f

CRGB leds[NUM_LEDS];
Nunchuk nunchuk;

struct Cascade {
    bool active;
    uint32_t startMs;
    uint16_t durationMs;
};

Cascade cascade = {false, 0, CASCADE_MAX_MS};

float accelBaselineZ = 512.0f;
bool tapArmed = false;
int16_t tapPeakDelta = 0;
uint32_t lastTapMs = 0;

void printNunchukInputs();
uint16_t durationFromTapPeak(int16_t peakDelta);
void startCascade(int16_t peakDelta);
void updateTapDetector();
void updateCascade();
void renderCascade(float headPos);
void pulseStrobes();

void setup() {
    Serial.begin(115200);
    delay(100);

    Serial.println(F("DoofStick cascade setup"));
    Serial.print(F("NUM_LEDS: "));
    Serial.println(NUM_LEDS);

    nunchuk.begin();
    while (!nunchuk.connect()) {
        Serial.println(F("Nunchuk not detected - check I2C wiring (SDA/SCL)."));
        delay(1000);
    }
    Serial.println(F("Nunchuk connected"));

    pinMode(LEFT_STROBE, OUTPUT);
    pinMode(MIDDLE_STROBE, OUTPUT);
    pinMode(RIGHT_STROBE, OUTPUT);
    digitalWrite(LEFT_STROBE, LOW);
    digitalWrite(MIDDLE_STROBE, LOW);
    digitalWrite(RIGHT_STROBE, LOW);

    FastLED.addLeds<WS2812, DATA1>(leds, NUM_LEDS);
    FastLED.addLeds<WS2812, DATA2>(leds, NUM_LEDS);
    FastLED.addLeds<WS2812, DATA3>(leds, NUM_LEDS);
    FastLED.addLeds<WS2812, DATA4>(leds, NUM_LEDS);
    FastLED.addLeds<WS2812, DATA5>(leds, NUM_LEDS);
    FastLED.addLeds<WS2812, DATA6>(leds, NUM_LEDS);
    FastLED.addLeds<WS2812, DATA7>(leds, NUM_LEDS);
    FastLED.addLeds<WS2812, DATA8>(leds, NUM_LEDS);

    FastLED.clear(true);
    FastLED.setBrightness(180);

    accelBaselineZ = nunchuk.accelZ();
    Serial.println(F("Ready - tap on Z axis to drive cascade"));
}

void loop() {
    if (!nunchuk.update()) {
        Serial.println(F("Nunchuk disconnected - reconnecting..."));
        delay(1000);
        nunchuk.connect();
        return;
    }

    updateTapDetector();
    updateCascade();

    EVERY_N_MILLISECONDS(500) {
        printNunchukInputs();
    }
}

uint16_t durationFromTapPeak(int16_t peakDelta) {
    peakDelta = constrain(peakDelta, TAP_PEAK_MIN, TAP_PEAK_MAX);
    return map(peakDelta, TAP_PEAK_MIN, TAP_PEAK_MAX, CASCADE_MAX_MS, CASCADE_MIN_MS);
}

void startCascade(int16_t peakDelta) {
    cascade.active = true;
    cascade.startMs = millis();
    cascade.durationMs = durationFromTapPeak(peakDelta);
    pulseStrobes();

    Serial.print(F("Beat Z delta="));
    Serial.print(peakDelta);
    Serial.print(F(" cascade_ms="));
    Serial.println(cascade.durationMs);
}

void updateTapDetector() {
    const int16_t az = nunchuk.accelZ();
    accelBaselineZ += (az - accelBaselineZ) * BASELINE_ALPHA;

    const int16_t delta = az - (int16_t)(accelBaselineZ + 0.5f);
    const int16_t absDelta = abs(delta);

    if (!tapArmed) {
        if (absDelta >= TAP_ARM_DELTA) {
            tapArmed = true;
            tapPeakDelta = absDelta;
        }
        return;
    }

    if (absDelta > tapPeakDelta) {
        tapPeakDelta = absDelta;
    }

    if (absDelta > TAP_RELEASE_DELTA) {
        return;
    }

    tapArmed = false;
    const uint32_t now = millis();
    if (now - lastTapMs < TAP_DEBOUNCE_MS) {
        return;
    }
    lastTapMs = now;
    startCascade(tapPeakDelta);
}

void updateCascade() {
    if (!cascade.active) {
        fadeToBlackBy(leds, NUM_LEDS, 48);
        FastLED.show();
        return;
    }

    const uint32_t elapsed = millis() - cascade.startMs;
    float progress = (float)elapsed / (float)cascade.durationMs;

    if (progress >= 1.0f) {
        progress = 1.0f;
        cascade.active = false;
    }

    renderCascade(progress * (float)NUM_LEDS);
    FastLED.show();
}

void renderCascade(float headPos) {
    fill_solid(leds, NUM_LEDS, CRGB::Black);

    const int head = min((int)headPos, NUM_LEDS - 1);
    const int tailStart = max(0, head - CASCADE_TAIL);

    for (int i = tailStart; i <= head; i++) {
        const uint8_t dist = head - i;
        const uint8_t brightness = 255 - scale8(dist, 255 / max(1, CASCADE_TAIL));
        leds[i] = CRGB(0, brightness / 2, brightness);
    }
}

void pulseStrobes() {
    digitalWrite(LEFT_STROBE, HIGH);
    digitalWrite(MIDDLE_STROBE, HIGH);
    digitalWrite(RIGHT_STROBE, HIGH);
    delay(12);
    digitalWrite(LEFT_STROBE, LOW);
    digitalWrite(MIDDLE_STROBE, LOW);
    digitalWrite(RIGHT_STROBE, LOW);
}

void printNunchukInputs() {
    Serial.println(F("--- Nunchuk ---"));
    Serial.print(F("  Accel Z:    "));
    Serial.print(nunchuk.accelZ());
    Serial.print(F("  baseline: "));
    Serial.println((int16_t)(accelBaselineZ + 0.5f));
    Serial.print(F("  Cascade:    "));
    if (cascade.active) {
        Serial.print(F("running "));
        Serial.print(cascade.durationMs);
        Serial.println(F(" ms"));
    } else {
        Serial.println(F("idle"));
    }
    Serial.println();
}
