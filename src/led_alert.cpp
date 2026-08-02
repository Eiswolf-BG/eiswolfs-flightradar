#include "led_alert.h"

namespace LedAlert {

namespace {
    constexpr uint8_t PIN_RED   = 4;
    constexpr uint8_t PIN_GREEN = 16;
    constexpr uint8_t PIN_BLUE  = 17;
    constexpr uint32_t BLINK_INTERVAL_MS = 400;

    bool initialized = false;
    bool blinkState = false;
    uint32_t lastToggleMs = 0;
}

void begin() {
    pinMode(PIN_RED, OUTPUT);
    pinMode(PIN_GREEN, OUTPUT);
    pinMode(PIN_BLUE, OUTPUT);
    digitalWrite(PIN_RED, HIGH);   // aus (LED ist active-low)
    digitalWrite(PIN_GREEN, HIGH); // aus
    digitalWrite(PIN_BLUE, HIGH);  // aus
    initialized = true;
}

bool update(bool active, uint32_t nowMs) {
    if (!initialized) begin();

    if (!active) {
        digitalWrite(PIN_GREEN, HIGH); // aus
        blinkState = false;
        return false;
    }

    if (nowMs - lastToggleMs >= BLINK_INTERVAL_MS) {
        lastToggleMs = nowMs;
        blinkState = !blinkState;
        digitalWrite(PIN_GREEN, blinkState ? LOW : HIGH); // LOW = an, volle Helligkeit (kein PWM noetig)
    }
    return blinkState;
}

}