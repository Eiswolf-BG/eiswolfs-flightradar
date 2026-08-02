#include "splash_screen.h"

namespace SplashScreen {

namespace {
    constexpr uint32_t MIN_DISPLAY_MS = 5000;
    uint32_t startMs = 0;

    constexpr int16_t STATUS_LINE_H = 18;
    constexpr int16_t STATUS_START_Y = 260;
    constexpr uint8_t MAX_STATUS_LINES = 3;

    // Einfache Vektor-Silhouette eines Flugzeugs von oben (keine Bilddatei
    // eingebunden, daher per Dreiecken gezeichnet) - in Gruen, wie der
    // Radar-Sweep-Strahl, passend zum restlichen Dark-Theme.
    void drawAirplane(TFT_eSPI& tft, int16_t cx) {
        uint16_t color = TFT_GREEN;

        // Rumpf
        tft.fillTriangle(cx, 110, cx - 12, 240, cx + 12, 240, color);

        // Haupttragflaechen
        tft.fillTriangle(cx, 170, cx - 110, 230, cx + 110, 230, color);

        // Leitwerk (Heckfluegel)
        tft.fillTriangle(cx, 232, cx - 35, 250, cx + 35, 250, color);
    }
}

void begin(TFT_eSPI& tft) {
    startMs = millis();

    int16_t cx = tft.width() / 2;

    tft.fillScreen(TFT_BLACK);
    drawAirplane(tft, cx);

    tft.setTextDatum(MC_DATUM);

    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(3);
    tft.drawString("Eiswolfs", cx, 40);

    tft.setTextSize(2);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.drawString("Flightradar", cx, 78);

    tft.setTextDatum(TL_DATUM);
    tft.setTextSize(1);
}

void setStatusLine(TFT_eSPI& tft, uint8_t slot, const String& text, uint16_t color) {
    if (slot >= MAX_STATUS_LINES) return;

    int16_t y = STATUS_START_Y + slot * STATUS_LINE_H;
    tft.fillRect(0, y, tft.width(), STATUS_LINE_H, TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(color, TFT_BLACK);
    tft.drawString(text, tft.width() / 2, y + STATUS_LINE_H / 2);
    tft.setTextDatum(TL_DATUM);
}

void waitRemaining() {
    uint32_t elapsed = millis() - startMs;
    if (elapsed < MIN_DISPLAY_MS) {
        delay(MIN_DISPLAY_MS - elapsed);
    }
}

}