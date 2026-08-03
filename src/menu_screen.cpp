#include "menu_screen.h"
#include "touch_input.h"
#include "calibration_screen.h"
#include "wifi_manage_screen.h"
#include "stats_screen.h"
#include "logbook_files_screen.h"
#include "settings_store.h"
#include "config.h"

namespace MenuScreen {

namespace {
    struct Rect {
        int16_t x, y, w, h;
        bool contains(int16_t px, int16_t py) const {
            return px >= x && px < x + w && py >= y && py < y + h;
        }
    };

    constexpr int16_t ROW_H = 26;
    constexpr int16_t ROW_GAP = 3;
    constexpr int16_t ROW_START_Y = 24;

    Rect rowRect(uint8_t index) {
        return {10, (int16_t)(ROW_START_Y + index * (ROW_H + ROW_GAP)),
                (int16_t)(Config::SCREEN_WIDTH - 20), ROW_H};
    }

    void drawButton(TFT_eSPI& tft, const Rect& r, const String& label) {
        tft.fillRoundRect(r.x, r.y, r.w, r.h, 4, TFT_NAVY);
        tft.drawRoundRect(r.x, r.y, r.w, r.h, 4, TFT_DARKGREY);
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(TFT_WHITE, TFT_NAVY);
        tft.drawString(label, r.x + r.w / 2, r.y + r.h / 2);
        tft.setTextDatum(TL_DATUM);
    }

    String onOff(bool on) { return on ? "ON" : "OFF"; }
}

void run(TFT_eSPI& tft) {
    Rect calibBtn      = rowRect(0);
    Rect invertBtn     = rowRect(1);
    Rect wifiBtn       = rowRect(2);
    Rect emergencyBtn  = rowRect(3);
    Rect proximityBtn  = rowRect(4);
    Rect logbookBtn    = rowRect(5);
    Rect statsBtn      = rowRect(6);
    Rect logFilesBtn   = rowRect(7);
    Rect backBtn       = rowRect(8);

    bool done = false;
    while (!done) {
        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.setCursor(10, 6);
        tft.println("Settings");

        drawButton(tft, calibBtn, "Calibrate touch");

        String invertLabel = SettingsStore::displayInverted()
                                  ? "Display: inverted (tap)"
                                  : "Display: normal (tap)";
        drawButton(tft, invertBtn, invertLabel);

        drawButton(tft, wifiBtn, "Manage WiFi networks");

        drawButton(tft, emergencyBtn, "Emergency alert: " + onOff(SettingsStore::emergencyAlertEnabled()));
        drawButton(tft, proximityBtn, "Proximity LED: " + onOff(SettingsStore::proximityAlertEnabled()));
        drawButton(tft, logbookBtn, "Flight logbook: " + onOff(SettingsStore::flightLogbookEnabled()));
        drawButton(tft, statsBtn, "Statistics");
        drawButton(tft, logFilesBtn, "Logbook files");

        drawButton(tft, backBtn, "Back");

        TouchInput::Point tap;
        while (true) {
            if (TouchInput::wasTapped(tap)) break;
            delay(20);
        }

        if (calibBtn.contains(tap.x, tap.y)) {
            CalibrationScreen::run(tft);
        } else if (invertBtn.contains(tap.x, tap.y)) {
            bool newState = !SettingsStore::displayInverted();
            SettingsStore::setDisplayInverted(newState);
            tft.invertDisplay(newState);
        } else if (wifiBtn.contains(tap.x, tap.y)) {
            WifiManageScreen::run(tft);
        } else if (emergencyBtn.contains(tap.x, tap.y)) {
            SettingsStore::setEmergencyAlertEnabled(!SettingsStore::emergencyAlertEnabled());
        } else if (proximityBtn.contains(tap.x, tap.y)) {
            SettingsStore::setProximityAlertEnabled(!SettingsStore::proximityAlertEnabled());
        } else if (logbookBtn.contains(tap.x, tap.y)) {
            SettingsStore::setFlightLogbookEnabled(!SettingsStore::flightLogbookEnabled());
        } else if (statsBtn.contains(tap.x, tap.y)) {
            StatsScreen::run(tft);
        } else if (logFilesBtn.contains(tap.x, tap.y)) {
            LogbookFilesScreen::run(tft);
        } else if (backBtn.contains(tap.x, tap.y)) {
            done = true;
        }
    }
}

}