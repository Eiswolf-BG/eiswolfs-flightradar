#include "menu_screen.h"
#include "touch_input.h"
#include "calibration_screen.h"
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

    void drawButton(TFT_eSPI& tft, const Rect& r, const String& label) {
        tft.fillRoundRect(r.x, r.y, r.w, r.h, 4, TFT_NAVY);
        tft.drawRoundRect(r.x, r.y, r.w, r.h, 4, TFT_DARKGREY);
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(TFT_WHITE, TFT_NAVY);
        tft.drawString(label, r.x + r.w / 2, r.y + r.h / 2);
        tft.setTextDatum(TL_DATUM);
    }
}

void run(TFT_eSPI& tft) {
    Rect calibBtn = {10, 60, Config::SCREEN_WIDTH - 20, 40};
    Rect invertBtn = {10, 112, Config::SCREEN_WIDTH - 20, 40};
    Rect backBtn  = {10, 260, Config::SCREEN_WIDTH - 20, 40};

    bool done = false;
    while (!done) {
        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.setCursor(10, 10);
        tft.println("Einstellungen");

        drawButton(tft, calibBtn, "Touch kalibrieren");
        String invertLabel = SettingsStore::displayInverted()
                                  ? "Display: invertiert (antippen)"
                                  : "Display: normal (antippen)";
        drawButton(tft, invertBtn, invertLabel);
        drawButton(tft, backBtn, "Zurueck");

        // Auf genau einen Tap warten
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
        } else if (backBtn.contains(tap.x, tap.y)) {
            done = true;
        }
    }
}

}