#pragma once
#include <Arduino.h>
#include <TFT_eSPI.h>

namespace RadarScreen {
    void render(TFT_eSPI& tft, int16_t top);
    void tick(TFT_eSPI& tft, int16_t top, uint32_t deltaMs);
    bool handleTap(int16_t x, int16_t y, int16_t top);
    void updateProximityAlert(uint32_t nowMs);

    struct EmergencyInfo {
        bool active = false;
        char callsign[9] = {0};
        char squawk[5] = {0};
    };

    EmergencyInfo checkEmergency();
}