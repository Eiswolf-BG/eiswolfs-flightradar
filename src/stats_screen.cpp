#include "stats_screen.h"
#include "flight_logbook.h"
#include "touch_input.h"
#include "config.h"

namespace StatsScreen {

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

    void drawStatRow(TFT_eSPI& tft, int16_t y, const String& label, const String& value) {
        tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
        tft.setCursor(10, y);
        tft.print(label);
        tft.setTextColor(TFT_GREEN, TFT_BLACK);
        tft.setTextSize(2);
        tft.setCursor(10, y + 14);
        tft.print(value);
        tft.setTextSize(1);
    }
}

void run(TFT_eSPI& tft) {
    uint16_t today = FlightLogbook::todayCount();
    uint32_t allTimeAircraft = 0;
    uint16_t allTimeDays = 0;
    FlightLogbook::computeAllTimeStats(allTimeAircraft, allTimeDays);

    Rect backBtn = {10, (int16_t)(Config::SCREEN_HEIGHT - 50), (int16_t)(Config::SCREEN_WIDTH - 20), 40};

    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(10, 10);
    tft.println("Statistics");

    drawStatRow(tft, 44, "Aircraft logged today", String(today));
    drawStatRow(tft, 90, "Aircraft logged all-time", String(allTimeAircraft));
    drawStatRow(tft, 136, "Days with sightings", String(allTimeDays));

    if (allTimeDays > 0) {
        float avgPerDay = (float)allTimeAircraft / (float)allTimeDays;
        char buf[16];
        snprintf(buf, sizeof(buf), "%.1f", avgPerDay);
        drawStatRow(tft, 182, "Average per day", String(buf));
    }

    drawButton(tft, backBtn, "Back");

    bool done = false;
    while (!done) {
        TouchInput::Point tap;
        if (TouchInput::wasTapped(tap) && backBtn.contains(tap.x, tap.y)) {
            done = true;
        }
        delay(20);
    }
}

}