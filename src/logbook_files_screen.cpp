#include "logbook_files_screen.h"
#include "flight_logbook.h"
#include "touch_input.h"
#include "config.h"

namespace LogbookFilesScreen {

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

    constexpr uint8_t MAX_DAYS_QUERIED = 31;
    constexpr uint8_t VISIBLE_ROWS = 10;
}

void run(TFT_eSPI& tft) {
    FlightLogbook::DayEntry days[MAX_DAYS_QUERIED];
    uint8_t count = FlightLogbook::listDays(days, MAX_DAYS_QUERIED);

    Rect backBtn = {10, (int16_t)(Config::SCREEN_HEIGHT - 50), (int16_t)(Config::SCREEN_WIDTH - 20), 40};

    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(10, 8);
    tft.println("Logbook files");

    if (count == 0) {
        tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
        tft.setCursor(10, 40);
        tft.println("No logbook entries yet.");
    } else {
        uint8_t startIdx = (count > VISIBLE_ROWS) ? (count - VISIBLE_ROWS) : 0;
        int16_t y = 30;

        if (count > VISIBLE_ROWS) {
            tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
            tft.setCursor(10, y);
            tft.printf("Showing last %d of %d days", VISIBLE_ROWS, count);
            y += 16;
        }

        for (uint8_t i = startIdx; i < count; i++) {
            tft.setTextColor(TFT_GREEN, TFT_BLACK);
            tft.setCursor(10, y);
            tft.print(days[i].date);
            tft.setTextColor(TFT_WHITE, TFT_BLACK);
            tft.setCursor(110, y);
            tft.printf("%lu aircraft", (unsigned long)days[i].count);
            y += 20;
        }
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