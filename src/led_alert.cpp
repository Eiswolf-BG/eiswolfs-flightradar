#include "led_alert.h"

namespace LedAlert {

namespace {
    constexpr uint8_t PIN_RED   = 4;
    constexpr uint8_t PIN_GREEN = 16;
    constexpr uint8_t PIN_BLUE  = 17;

    constexpr uint32_t GREEN_BLINK_INTERVAL_MS = 400;
    constexpr uint32_t RED_BLINK_INTERVAL_MS   = 150;

    bool initialized = false;
    bool blinkState = false;
    uint32_t lastToggleMs = 0;
    Mode lastMode = Mode::Off;

    void setAllOff() {
        digitalWrite(PIN_RED, HIGH);
        digitalWrite(PIN_GREEN, HIGH);
        digitalWrite(PIN_BLUE, HIGH);
    }
}

void begin() {
    pinMode(PIN_RED, OUTPUT);
    pinMode(PIN_GREEN, OUTPUT);
    pinMode(PIN_BLUE, OUTPUT);
    setAllOff();
    initialized = true;
}

bool update(Mode mode, uint32_t nowMs) {
    if (!initialized) begin();

    if (mode == Mode::Off) {
        setAllOff();
        blinkState = false;
        lastMode = mode;
        return false;
    }

    if (mode != lastMode) {
        lastToggleMs = nowMs;
        blinkState = true;
        lastMode = mode;
    }

    uint32_t interval = (mode == Mode::EmergencyRed) ? RED_BLINK_INTERVAL_MS : GREEN_BLINK_INTERVAL_MS;
    uint8_t pin = (mode == Mode::EmergencyRed) ? PIN_RED : PIN_GREEN;

    if (nowMs - lastToggleMs >= interval) {
        lastToggleMs = nowMs;
        blinkState = !blinkState;
    }

    digitalWrite(pin, blinkState ? LOW : HIGH);
    digitalWrite(mode == Mode::EmergencyRed ? PIN_GREEN : PIN_RED, HIGH);
    digitalWrite(PIN_BLUE, HIGH);

    return blinkState;
}

}