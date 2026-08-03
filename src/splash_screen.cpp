#include "splash_screen.h"

namespace SplashScreen {

namespace {
    constexpr uint32_t MIN_DISPLAY_MS = 5000;
    uint32_t startMs = 0;

    constexpr int16_t STATUS_LINE_H = 18;
    constexpr int16_t STATUS_START_Y = 260;
    constexpr uint8_t MAX_STATUS_LINES = 3;

    // Dezente konzentrische Ringe + Fadenkreuz als "Zielfernrohr"-Hintergrund,
    // in gedaempftem Gruen (gleicher Ton wie die Sweep-Nachzieh-Linie im
    // Radar-Bildschirm), damit das Flugzeug wie ins Visier genommen wirkt.
    void drawRadarReticle(TFT_eSPI& tft, int16_t cx, int16_t cy) {
        uint16_t dim = 0x0320;
        tft.drawCircle(cx, cy, 112, dim);
        tft.drawCircle(cx, cy, 76, dim);
        tft.drawCircle(cx, cy, 40, dim);
        tft.drawFastHLine(cx - 112, cy, 224, dim);
        tft.drawFastVLine(cx, cy - 112, 224, dim);
    }

    // Einfache Vektor-Silhouette eines schlanken Duesenjets von oben (keine
    // Bilddatei eingebunden, daher per Dreiecken gezeichnet) - nur als Umriss
    // (nicht ausgefuellt), in Gruen, passend zum restlichen Dark-Theme.
    void drawAirplane(TFT_eSPI& tft, int16_t cx) {
        uint16_t color = TFT_GREEN;

        // Schlanker Rumpf (nur Umriss)
        tft.drawTriangle(cx, 105, cx - 6, 255, cx + 6, 255, color);

        // Deltafluegel, nach hinten gepfeilt (nur Umriss)
        tft.drawTriangle(cx, 160, cx - 100, 235, cx, 200, color);
        tft.drawTriangle(cx, 160, cx + 100, 235, cx, 200, color);

        // Kleines Leitwerk (Hoehenruder) am Heck (nur Umriss)
        tft.drawTriangle(cx, 250, cx - 22, 268, cx + 22, 268, color);
    }
}

void begin(TFT_eSPI& tft) {
    startMs = millis();

    int16_t cx = tft.width() / 2;

    tft.fillScreen(TFT_BLACK);
    drawRadarReticle(tft, cx, 185);
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