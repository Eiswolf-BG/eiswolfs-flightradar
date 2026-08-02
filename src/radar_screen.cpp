#include "radar_screen.h"
#include "config.h"
#include "aircraft.h"
#include "aircraft_table.h"
#include "airline_lookup.h"
#include "aircraft_details.h"
#include "radar_math.h"
#include "units.h"
#include "settings_store.h"
#include "led_alert.h"
#include <math.h>

namespace RadarScreen {

namespace {
    struct Rect {
        int16_t x, y, w, h;
        bool contains(int16_t px, int16_t py) const {
            return px >= x && px < x + w && py >= y && py < y + h;
        }
    };

    struct Layout {
        int16_t cx, cy, radius;
        Rect rangeBtn;
        int16_t infoTop;
    };

    constexpr int16_t INFO_BAR_H = 50;

    Layout computeLayout(int16_t top) {
        Layout L;
        L.infoTop = Config::SCREEN_HEIGHT - INFO_BAR_H;
        int16_t maxRadiusByWidth = Config::SCREEN_WIDTH / 2 - 6;
        int16_t maxRadiusByHeight = (L.infoTop - top) / 2 - 6;
        L.radius = min(maxRadiusByWidth, maxRadiusByHeight);
        L.cx = Config::SCREEN_WIDTH / 2;
        L.cy = top + L.radius + 6;
        L.rangeBtn = {(int16_t)(Config::SCREEN_WIDTH - 70), (int16_t)(L.infoTop + 4), 62, 22};
        return L;
    }

    constexpr int16_t DETAIL_PANEL_H = 150;

    struct HitPoint {
        int16_t x, y;
        bool valid;
        char hex[7];
        char callsign[9];
        uint16_t color;
        float headingDeg;
        float distanceKm;
    };
    constexpr uint8_t MAX_HIT_POINTS = Config::MAX_TRACKED_AIRCRAFT;
    HitPoint hitPoints[MAX_HIT_POINTS];

    char selectedHex[7] = {0};

    bool ledBlinkOn = true;

    float sweepAngleDeg = 0.0f;
    float prevSweepAngleDeg = -1.0f;
    constexpr float SWEEP_DEGREES_PER_SEC = 45.0f;

    uint16_t colorForAltitude(int32_t altFt) {
        if (altFt < Config::COLOR_LOW_ALT_THRESHOLD_FT) return TFT_GREEN;
        if (altFt < Config::COLOR_MID_ALT_THRESHOLD_FT) return TFT_YELLOW;
        return TFT_RED;
    }

    void printLineTruncated(TFT_eSPI& gfx, int16_t x, int16_t y, int16_t maxWidth, const String& text) {
        String s = text;
        if (gfx.textWidth(s) > maxWidth) {
            while (s.length() > 1 && gfx.textWidth(s + "...") > maxWidth) {
                s.remove(s.length() - 1);
            }
            s += "...";
        }
        gfx.setCursor(x, y);
        gfx.print(s);
    }

    void drawButton(TFT_eSPI& gfx, const Rect& r, const String& label) {
        gfx.fillRoundRect(r.x, r.y, r.w, r.h, 4, TFT_NAVY);
        gfx.drawRoundRect(r.x, r.y, r.w, r.h, 4, TFT_DARKGREY);
        gfx.setTextDatum(MC_DATUM);
        gfx.setTextColor(TFT_WHITE, TFT_NAVY);
        gfx.drawString(label, r.x + r.w / 2, r.y + r.h / 2);
        gfx.setTextDatum(TL_DATUM);
    }

    void drawLegend(TFT_eSPI& gfx, int16_t y) {
        struct { uint16_t color; const char* label; } items[3] = {
            {TFT_GREEN,  "<10k ft"},
            {TFT_YELLOW, "10-30k"},
            {TFT_RED,    ">30k ft"},
        };
        int16_t segW = Config::SCREEN_WIDTH / 3;
        gfx.setTextColor(TFT_WHITE, TFT_BLACK);
        for (uint8_t i = 0; i < 3; i++) {
            int16_t x0 = i * segW + 6;
            gfx.fillCircle(x0, y + 4, 3, items[i].color);
            gfx.setCursor(x0 + 7, y);
            gfx.print(items[i].label);
        }
    }

    void drawSweepLine(TFT_eSPI& gfx, const Layout& L, float angleDeg, uint16_t color) {
        double rad = angleDeg * DEG_TO_RAD;
        int16_t x2 = L.cx + (int16_t)(L.radius * sin(rad));
        int16_t y2 = L.cy - (int16_t)(L.radius * cos(rad));
        gfx.drawLine(L.cx, L.cy, x2, y2, color);
    }

    void drawStaticBackground(TFT_eSPI& gfx, const Layout& L, float rangeKm) {
        gfx.drawCircle(L.cx, L.cy, L.radius, TFT_DARKGREY);
        gfx.drawCircle(L.cx, L.cy, L.radius * 2 / 3, TFT_DARKGREY);
        gfx.drawCircle(L.cx, L.cy, L.radius / 3, TFT_DARKGREY);
        gfx.drawFastHLine(L.cx - L.radius, L.cy, L.radius * 2, TFT_DARKGREY);
        gfx.drawFastVLine(L.cx, L.cy - L.radius, L.radius * 2, TFT_DARKGREY);

        gfx.setTextColor(TFT_DARKGREY, TFT_BLACK);
        gfx.setTextDatum(MC_DATUM);
        gfx.drawString("N", L.cx, L.cy - L.radius - 8);
        char ringLabel[8];
        snprintf(ringLabel, sizeof(ringLabel), "%.0f", rangeKm / 3);
        gfx.drawString(ringLabel, L.cx, L.cy - L.radius / 3);
        snprintf(ringLabel, sizeof(ringLabel), "%.0f", rangeKm * 2 / 3);
        gfx.drawString(ringLabel, L.cx, L.cy - L.radius * 2 / 3);
        gfx.setTextDatum(TL_DATUM);
    }

    struct PanelState {
        bool valid = false;
        char hex[7] = {0};
        String callsignText, airlineText, modelText,
               altText, speedText, distHeadingText, seatsText;
    };
    PanelState lastPanel;

    void updateLine(TFT_eSPI& gfx, int16_t y, int16_t h, int16_t maxWidth,
                     uint16_t fg, String& cached, const String& newText, bool forceFull) {
        if (!forceFull && cached == newText) return;
        gfx.fillRect(0, y - 2, Config::SCREEN_WIDTH, h, TFT_NAVY);
        gfx.setTextColor(fg, TFT_NAVY);
        printLineTruncated(gfx, 8, y, maxWidth, newText);
        cached = newText;
    }

    void drawDetailPanel(TFT_eSPI& gfx, Aircraft& a) {
        AirlineLookup::resolve(a);
        AircraftDetails::Info details = AircraftDetails::get(a.hex);

        int16_t panelTop = Config::SCREEN_HEIGHT - DETAIL_PANEL_H;
        constexpr int16_t textMaxWidth = Config::SCREEN_WIDTH - 16;

        bool forceFull = !lastPanel.valid || strcmp(lastPanel.hex, a.hex) != 0;
        if (forceFull) {
            gfx.fillRect(0, panelTop, Config::SCREEN_WIDTH, DETAIL_PANEL_H, TFT_NAVY);
            gfx.drawRect(0, panelTop, Config::SCREEN_WIDTH, DETAIL_PANEL_H, TFT_DARKGREY);
            lastPanel = PanelState{};
            strncpy(lastPanel.hex, a.hex, sizeof(lastPanel.hex) - 1);
        }

        int16_t y = panelTop + 6;

        {
            String txt = a.callsign[0] ? a.callsign : a.hex;
            if (forceFull || lastPanel.callsignText != txt) {
                gfx.fillRect(0, y - 2, Config::SCREEN_WIDTH, 20, TFT_NAVY);
                gfx.setTextColor(TFT_WHITE, TFT_NAVY);
                gfx.setTextSize(2);
                printLineTruncated(gfx, 8, y, textMaxWidth, txt);
                gfx.setTextSize(1);
                lastPanel.callsignText = txt;
            }
        }
        y += 22;

        updateLine(gfx, y, 15, textMaxWidth, TFT_GREEN, lastPanel.airlineText,
                   String(a.airlineName), forceFull);
        y += 15;

        String modelLine;
        if (details.loading) {
            modelLine = "Model: loading...";
        } else if (details.model[0]) {
            modelLine = String("Model: ") + details.model;
        } else {
            modelLine = String("Type: ") + (a.typeCode[0] ? a.typeCode : "unknown");
        }
        updateLine(gfx, y, 15, textMaxWidth, TFT_WHITE, lastPanel.modelText, modelLine, forceFull);
        y += 15;

        char buf[40];
        snprintf(buf, sizeof(buf), "Alt: %.0fm / %.0fft",
                 Units::feetToMeters((float)a.altBaroFt), (float)a.altBaroFt);
        updateLine(gfx, y, 15, textMaxWidth, TFT_WHITE, lastPanel.altText, String(buf), forceFull);
        y += 15;

        snprintf(buf, sizeof(buf), "Speed: %.0fkm/h / %.0fkt",
                 Units::ktToKmh(a.groundSpeedKt), a.groundSpeedKt);
        updateLine(gfx, y, 15, textMaxWidth, TFT_WHITE, lastPanel.speedText, String(buf), forceFull);
        y += 15;

        snprintf(buf, sizeof(buf), "Dist: %.0fkm / %.0fnm   Hdg: %.0f",
                 a.distanceKm, Units::kmToNm(a.distanceKm), a.headingDeg);
        updateLine(gfx, y, 15, textMaxWidth, TFT_WHITE, lastPanel.distHeadingText, String(buf), forceFull);
        y += 15;

        String seatsLine = a.estSeats > 0
            ? String("Seats (estimated): ") + a.estSeats
            : String("Seats: unknown");
        updateLine(gfx, y, 15, textMaxWidth, TFT_WHITE, lastPanel.seatsText, seatsLine, forceFull);
        y += 18;

        if (forceFull) {
            gfx.setTextColor(TFT_DARKGREY, TFT_NAVY);
            gfx.setCursor(8, y);
            gfx.print("Tap elsewhere to close");
        }

        lastPanel.valid = true;
    }
}

void render(TFT_eSPI& tft, int16_t top) {
    Layout L = computeLayout(top);
    float rangeKm = Config::RANGE_STEPS_KM[SettingsStore::rangeIndex()];

    bool panelAlreadyOpen = selectedHex[0] && lastPanel.valid &&
                            strcmp(lastPanel.hex, selectedHex) == 0;

    if (panelAlreadyOpen) {
        AircraftTable::lock();
        Aircraft* table = AircraftTable::raw();
        Aircraft selected{};
        bool found = false;
        for (uint8_t i = 0; i < AircraftTable::capacity(); i++) {
            if (table[i].valid && strcmp(table[i].hex, selectedHex) == 0) {
                selected = table[i];
                found = true;
                break;
            }
        }
        AircraftTable::unlock();

        if (found && selected.distanceKm <= rangeKm * 1.05f) {
            tft.startWrite();
            drawDetailPanel(tft, selected);
            tft.endWrite();
            return;
        }
        selectedHex[0] = 0;
        lastPanel.valid = false;
    }

    tft.startWrite();

    tft.fillRect(0, top, Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT - top, TFT_BLACK);

    drawStaticBackground(tft, L, rangeKm);

    drawSweepLine(tft, L, sweepAngleDeg, TFT_GREEN);
    prevSweepAngleDeg = sweepAngleDeg;

    tft.fillCircle(L.cx, L.cy, 3, TFT_WHITE);

    static Aircraft snapshot[Config::MAX_TRACKED_AIRCRAFT];
    uint8_t count = 0;

    AircraftTable::lock();
    Aircraft* table = AircraftTable::raw();
    for (uint8_t i = 0; i < AircraftTable::capacity(); i++) {
        if (table[i].valid) snapshot[count++] = table[i];
    }
    AircraftTable::unlock();

    bool selectionStillPresent = false;
    Aircraft selected{};

    for (uint8_t i = 0; i < MAX_HIT_POINTS; i++) hitPoints[i].valid = false;

    for (uint8_t i = 0; i < count && i < MAX_HIT_POINTS; i++) {
        Aircraft& a = snapshot[i];
        if (a.distanceKm > rangeKm * 1.05f) continue;

        RadarMath::PolarCoord polar{a.distanceKm, a.bearingDeg};
        RadarMath::ScreenPoint pt = RadarMath::toScreen(polar, L.cx, L.cy, L.radius, rangeKm);

        uint16_t color = colorForAltitude(a.altBaroFt);
        bool isSelected = selectedHex[0] && strcmp(a.hex, selectedHex) == 0;

        if (isSelected) {
            tft.drawCircle(pt.x, pt.y, 9, TFT_WHITE);
            selectionStillPresent = true;
            selected = a;
        }
        tft.fillCircle(pt.x, pt.y, 5, color);

        double rad = a.headingDeg * PI / 180.0;
        int16_t dx = (int16_t)(sin(rad) * 10);
        int16_t dy = (int16_t)(-cos(rad) * 10);
        tft.drawLine(pt.x, pt.y, pt.x + dx, pt.y + dy, color);

        tft.setTextColor(color, TFT_BLACK);
        tft.setTextDatum(BC_DATUM);
        const char* label = a.callsign[0] ? a.callsign : a.hex;
        tft.drawString(label, pt.x, pt.y - 8);
        tft.setTextDatum(TL_DATUM);

        hitPoints[i].x = pt.x;
        hitPoints[i].y = pt.y;
        hitPoints[i].valid = true;
        hitPoints[i].color = color;
        hitPoints[i].headingDeg = a.headingDeg;
        hitPoints[i].distanceKm = a.distanceKm;
        strncpy(hitPoints[i].hex, a.hex, sizeof(hitPoints[i].hex) - 1);
        strncpy(hitPoints[i].callsign, a.callsign, sizeof(hitPoints[i].callsign) - 1);
    }

    if (selectedHex[0] && !selectionStillPresent) {
        selectedHex[0] = 0;
    }

    if (selectionStillPresent) {
        tft.endWrite();
        drawDetailPanel(tft, selected);
        tft.startWrite();
    } else {
        lastPanel.valid = false;
        int16_t infoTop = L.infoTop;
        tft.drawFastHLine(0, infoTop, Config::SCREEN_WIDTH, TFT_DARKGREY);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.setCursor(8, infoTop + 6);
        tft.print("Tap for details");

        char rangeLabel[8];
        snprintf(rangeLabel, sizeof(rangeLabel), "%.0fkm", rangeKm);
        drawButton(tft, L.rangeBtn, rangeLabel);

        drawLegend(tft, infoTop + 30);
    }

    tft.endWrite();
}

void tick(TFT_eSPI& tft, int16_t top, uint32_t deltaMs) {
    if (selectedHex[0]) return;

    Layout L = computeLayout(top);
    float rangeKm = Config::RANGE_STEPS_KM[SettingsStore::rangeIndex()];

    if (prevSweepAngleDeg >= 0.0f) {
        drawSweepLine(tft, L, prevSweepAngleDeg, TFT_BLACK);
        drawStaticBackground(tft, L, rangeKm);
        tft.fillCircle(L.cx, L.cy, 3, TFT_WHITE);
    }

    sweepAngleDeg += SWEEP_DEGREES_PER_SEC * (deltaMs / 1000.0f);
    if (sweepAngleDeg >= 360.0f) sweepAngleDeg -= 360.0f;

    drawSweepLine(tft, L, sweepAngleDeg, TFT_GREEN);
    prevSweepAngleDeg = sweepAngleDeg;

    for (uint8_t i = 0; i < MAX_HIT_POINTS; i++) {
        if (!hitPoints[i].valid) continue;
        const HitPoint& hp = hitPoints[i];

        bool inAlertRange = hp.distanceKm <= Config::LED_ALERT_RADIUS_KM;
        if (inAlertRange && !ledBlinkOn) {
            tft.fillRect(hp.x - 20, hp.y - 18, 40, 30, TFT_BLACK);
            continue;
        }

        tft.fillCircle(hp.x, hp.y, 5, hp.color);

        double rad = hp.headingDeg * PI / 180.0;
        int16_t dx = (int16_t)(sin(rad) * 10);
        int16_t dy = (int16_t)(-cos(rad) * 10);
        tft.drawLine(hp.x, hp.y, hp.x + dx, hp.y + dy, hp.color);

        tft.setTextColor(hp.color, TFT_BLACK);
        tft.setTextDatum(BC_DATUM);
        const char* label = hp.callsign[0] ? hp.callsign : hp.hex;
        tft.drawString(label, hp.x, hp.y - 8);
        tft.setTextDatum(TL_DATUM);
    }
}

bool handleTap(int16_t x, int16_t y, int16_t top) {
    Layout L = computeLayout(top);

    if (selectedHex[0]) {
        for (uint8_t i = 0; i < MAX_HIT_POINTS; i++) {
            if (!hitPoints[i].valid) continue;
            int16_t dx = x - hitPoints[i].x;
            int16_t dy = y - hitPoints[i].y;
            if (dx * dx + dy * dy <= 12 * 12) {
                strncpy(selectedHex, hitPoints[i].hex, sizeof(selectedHex) - 1);
                AircraftDetails::request(hitPoints[i].hex);
                return true;
            }
        }
        selectedHex[0] = 0;
        lastPanel.valid = false;
        return true;
    }

    if (L.rangeBtn.contains(x, y)) {
        uint8_t idx = (SettingsStore::rangeIndex() + 1) % Config::RANGE_STEP_COUNT;
        SettingsStore::setRangeIndex(idx);
        return true;
    }

    for (uint8_t i = 0; i < MAX_HIT_POINTS; i++) {
        if (!hitPoints[i].valid) continue;
        int16_t dx = x - hitPoints[i].x;
        int16_t dy = y - hitPoints[i].y;
        if (dx * dx + dy * dy <= 12 * 12) {
            strncpy(selectedHex, hitPoints[i].hex, sizeof(selectedHex) - 1);
            AircraftDetails::request(hitPoints[i].hex);
            return true;
        }
    }

    return true;
}

void updateProximityAlert(uint32_t nowMs) {
    bool anyClose = false;

    AircraftTable::lock();
    Aircraft* table = AircraftTable::raw();
    for (uint8_t i = 0; i < AircraftTable::capacity(); i++) {
        if (table[i].valid && table[i].distanceKm <= Config::LED_ALERT_RADIUS_KM) {
            anyClose = true;
            break;
        }
    }
    AircraftTable::unlock();

    ledBlinkOn = LedAlert::update(anyClose, nowMs);
}

}