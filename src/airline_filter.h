#pragma once
#include <Arduino.h>

namespace AirlineFilter {
    constexpr uint8_t MAX_HIDDEN = 10;

    void init();

    uint8_t count();
    String icaoAt(uint8_t index);

    bool addHidden(const char* icaoPrefix);
    void removeHidden(uint8_t index);

    bool isHidden(const char* callsign);
}