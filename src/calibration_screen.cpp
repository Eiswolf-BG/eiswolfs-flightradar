#include "calibration_screen.h"
#include "touch_input.h"
#include "config.h"

namespace CalibrationScreen {

namespace {
    struct RawAvg {
        long sumX = 0, sumY = 0;
        uint16_t n = 0;
        void add(int16_t x, int16_t y) { sumX += x; sumY += y; n++; }
        int16_t avgX() const { return n ? (int16_t)(sumX / n) : 0; }
        int16_t avgY() const { return n ? (int16_t)(sumY / n) : 0; }
    };

    void drawTarget(TFT_eSPI& tft, int16_t x, int16_t y) {
        tft.fillCircle(x, y, 10, TFT_RED);
        tft.drawFastHLine(x - 16, y, 32, TFT_RED);
        tft.drawFastVLine(x, y - 16, 32, TFT_RED);
        tft.fillCircle(x, y, 3, TFT_WHITE);
    }

    // Wartet auf eine Beruehrung, mittelt ein paar Rohwerte waehrend sie
    // gehalten wird, und wartet danach auf das Loslassen, bevor es zurueckkehrt.
    RawAvg waitForTap() {
        RawAvg avg;

        while (!TouchInput::rawPoint().touched) {
            delay(10);
        }

        uint32_t sampleStart = millis();
        while (millis() - sampleStart < 250) {
            TouchInput::Point p = TouchInput::rawPoint();
            if (p.touched) avg.add(p.x, p.y);
            delay(10);
        }

        while (TouchInput::rawPoint().touched) {
            delay(10);
        }
        delay(150); // kleine Pause, damit der naechste Tap nicht sofort durchrutscht

        return avg;
    }

    void swapIfNeeded(int16_t& lo, int16_t& hi) {
        if (lo > hi) { int16_t t = lo; lo = hi; hi = t; }
    }
}

void run(TFT_eSPI& tft) {
    const int16_t M = 24;
    const int16_t W = Config::SCREEN_WIDTH;
    const int16_t H = Config::SCREEN_HEIGHT;

    struct { int16_t x, y; const char* label; } targets[4] = {
        { M,     M,     "Oben links" },
        { W - M, M,     "Oben rechts" },
        { W - M, H - M, "Unten rechts" },
        { M,     H - M, "Unten links" },
    };

    RawAvg samples[4];

    for (uint8_t i = 0; i < 4; i++) {
        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.setTextSize(1);
        // Text mittig zeichnen - weit weg von allen 4 Eck-Positionen, damit er
        // nie vom Ziel-Kreis ueberdeckt wird.
        tft.setTextDatum(MC_DATUM);
        tft.drawString("Touch-Kalibrierung", W / 2, H / 2 - 12);
        char msg[40];
        snprintf(msg, sizeof(msg), "Bitte Kreis beruehren:");
        tft.drawString(msg, W / 2, H / 2 + 4);
        tft.drawString(targets[i].label, W / 2, H / 2 + 20);
        tft.setTextDatum(TL_DATUM);

        drawTarget(tft, targets[i].x, targets[i].y);
        samples[i] = waitForTap();
    }

    int16_t xmin = (samples[0].avgX() + samples[3].avgX()) / 2; // links oben+unten
    int16_t xmax = (samples[1].avgX() + samples[2].avgX()) / 2; // rechts oben+unten
    int16_t ymin = (samples[0].avgY() + samples[1].avgY()) / 2; // oben links+rechts
    int16_t ymax = (samples[3].avgY() + samples[2].avgY()) / 2; // unten links+rechts

    swapIfNeeded(xmin, xmax);
    swapIfNeeded(ymin, ymax);

    TouchInput::setCalibration(xmin, xmax, ymin, ymax);
    TouchInput::saveCalibration();

    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(10, 10);
    tft.println("Kalibrierung gespeichert!");
    delay(800);
}

}