#include "menu_screen.h"
#include "touch_input.h"
#include "calibration_screen.h"
#include "wifi_manage_screen.h"
#include "stats_screen.h"
#include "logbook_files_screen.h"
#include "location_presets_screen.h"
#include "airline_filter_screen.h"
#include "language_screen.h"
#include "units_screen.h"
#include "settings_backup.h"
#include "settings_store.h"
#include "i18n.h"
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
    constexpr int16_t ROW_START_Y = 20;

    Rect rowRect(uint8_t index) {
        return {10, (int16_t)(ROW_START_Y + index * (ROW_H + ROW_GAP)),
                (int16_t)(Config::SCREEN_WIDTH - 20), ROW_H};
    }

    void drawButton(TFT_eSPI& tft, const Rect& r, const String& label, uint16_t bg = TFT_NAVY) {
        tft.fillRoundRect(r.x, r.y, r.w, r.h, 4, bg);
        tft.drawRoundRect(r.x, r.y, r.w, r.h, 4, TFT_DARKGREY);
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(TFT_WHITE, bg);
        tft.drawString(label, r.x + r.w / 2, r.y + r.h / 2);
        tft.setTextDatum(TL_DATUM);
    }

    String onOff(bool on) { return I18n::t(on ? StringId::ON : StringId::OFF); }

    String screenTimeoutLabel(uint8_t minutes) {
        String prefix = I18n::t(StringId::MENU_SCREEN_TIMEOUT_PREFIX);
        if (minutes == 0) return prefix + I18n::t(StringId::NEVER);
        return prefix + String(minutes) + " min";
    }

    void showBriefMessage(TFT_eSPI& tft, const String& msg, uint16_t color) {
        tft.fillRect(0, Config::SCREEN_HEIGHT - 18, Config::SCREEN_WIDTH, 18, TFT_BLACK);
        tft.setTextColor(color, TFT_BLACK);
        tft.setTextDatum(MC_DATUM);
        tft.drawString(msg, Config::SCREEN_WIDTH / 2, Config::SCREEN_HEIGHT - 9);
        tft.setTextDatum(TL_DATUM);
        delay(1200);
    }
}

void run(TFT_eSPI& tft) {
    uint8_t page = 0;
    bool done = false;

    while (!done) {
        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.setCursor(10, 2);
        tft.println(page == 0 ? I18n::t(StringId::MENU_SETTINGS) : I18n::t(StringId::MENU_SETTINGS_PAGE2));

        if (page == 0) {
            Rect calibBtn    = rowRect(0);
            Rect invertBtn   = rowRect(1);
            Rect wifiBtn     = rowRect(2);
            Rect locationBtn = rowRect(3);
            Rect timeoutBtn  = rowRect(4);
            Rect statsBtn    = rowRect(5);
            Rect logFilesBtn = rowRect(6);
            Rect moreBtn     = rowRect(7);
            Rect backBtn     = rowRect(8);

            drawButton(tft, calibBtn, I18n::t(StringId::MENU_CALIBRATE));

            String invertLabel = SettingsStore::displayInverted()
                                      ? I18n::t(StringId::MENU_DISPLAY_INVERTED)
                                      : I18n::t(StringId::MENU_DISPLAY_NORMAL);
            drawButton(tft, invertBtn, invertLabel);

            drawButton(tft, wifiBtn, I18n::t(StringId::MENU_MANAGE_WIFI));
            drawButton(tft, locationBtn, I18n::t(StringId::MENU_LOCATION_PRESETS));
            drawButton(tft, timeoutBtn, screenTimeoutLabel(SettingsStore::screenTimeoutMinutes()));
            drawButton(tft, statsBtn, I18n::t(StringId::MENU_STATISTICS));
            drawButton(tft, logFilesBtn, I18n::t(StringId::MENU_LOGBOOK_FILES));
            drawButton(tft, moreBtn, I18n::t(StringId::MENU_MORE_SETTINGS));
            drawButton(tft, backBtn, I18n::t(StringId::BACK));

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
            } else if (locationBtn.contains(tap.x, tap.y)) {
                LocationPresetsScreen::run(tft);
            } else if (timeoutBtn.contains(tap.x, tap.y)) {
                uint8_t current = SettingsStore::screenTimeoutMinutes();
                uint8_t next = (current >= 10) ? 0 : (current + 1);
                SettingsStore::setScreenTimeoutMinutes(next);
            } else if (statsBtn.contains(tap.x, tap.y)) {
                StatsScreen::run(tft);
            } else if (logFilesBtn.contains(tap.x, tap.y)) {
                LogbookFilesScreen::run(tft);
            } else if (moreBtn.contains(tap.x, tap.y)) {
                page = 1;
            } else if (backBtn.contains(tap.x, tap.y)) {
                done = true;
            }
        } else if (page == 1) {
            Rect emergencyBtn  = rowRect(0);
            Rect proximityBtn  = rowRect(1);
            Rect heartbeatBtn  = rowRect(2);
            Rect logbookBtn    = rowRect(3);
            Rect groundBtn     = rowRect(4);
            Rect airlineBtn    = rowRect(5);
            Rect moreBtn       = rowRect(6);
            Rect backBtn       = rowRect(7);

            drawButton(tft, emergencyBtn, I18n::t(StringId::MENU_EMERGENCY_ALERT) + onOff(SettingsStore::emergencyAlertEnabled()));
            drawButton(tft, proximityBtn, I18n::t(StringId::MENU_PROXIMITY_LED) + onOff(SettingsStore::proximityAlertEnabled()));
            drawButton(tft, heartbeatBtn, I18n::t(StringId::MENU_LED_HEARTBEAT) + onOff(SettingsStore::ledHeartbeatEnabled()));
            drawButton(tft, logbookBtn, I18n::t(StringId::MENU_FLIGHT_LOGBOOK) + onOff(SettingsStore::flightLogbookEnabled()));
            drawButton(tft, groundBtn, I18n::t(StringId::MENU_HIDE_GROUND) + onOff(SettingsStore::hideGroundVehicles()));
            drawButton(tft, airlineBtn, I18n::t(StringId::MENU_AIRLINE_FILTER));
            drawButton(tft, moreBtn, I18n::t(StringId::MENU_MORE_SETTINGS));
            drawButton(tft, backBtn, I18n::t(StringId::BACK_ARROW));

            TouchInput::Point tap;
            while (true) {
                if (TouchInput::wasTapped(tap)) break;
                delay(20);
            }

            if (emergencyBtn.contains(tap.x, tap.y)) {
                SettingsStore::setEmergencyAlertEnabled(!SettingsStore::emergencyAlertEnabled());
            } else if (proximityBtn.contains(tap.x, tap.y)) {
                SettingsStore::setProximityAlertEnabled(!SettingsStore::proximityAlertEnabled());
            } else if (heartbeatBtn.contains(tap.x, tap.y)) {
                SettingsStore::setLedHeartbeatEnabled(!SettingsStore::ledHeartbeatEnabled());
            } else if (logbookBtn.contains(tap.x, tap.y)) {
                SettingsStore::setFlightLogbookEnabled(!SettingsStore::flightLogbookEnabled());
            } else if (groundBtn.contains(tap.x, tap.y)) {
                SettingsStore::setHideGroundVehicles(!SettingsStore::hideGroundVehicles());
            } else if (airlineBtn.contains(tap.x, tap.y)) {
                AirlineFilterScreen::run(tft);
            } else if (moreBtn.contains(tap.x, tap.y)) {
                page = 2;
            } else if (backBtn.contains(tap.x, tap.y)) {
                page = 0;
            }
        } else {
            Rect languageBtn = rowRect(0);
            Rect unitsBtn     = rowRect(1);
            Rect backupBtn    = rowRect(2);
            Rect restoreBtn   = rowRect(3);
            Rect backBtn      = rowRect(4);

            drawButton(tft, languageBtn, String(I18n::t(StringId::MENU_LANGUAGE)) + ": " + I18n::languageName(SettingsStore::language()));
            drawButton(tft, unitsBtn, I18n::t(StringId::MENU_UNITS));
            drawButton(tft, backupBtn, I18n::t(StringId::MENU_BACKUP));
            drawButton(tft, restoreBtn, I18n::t(StringId::MENU_RESTORE),
                       SettingsBackup::hasBackup() ? TFT_NAVY : TFT_DARKGREY);
            drawButton(tft, backBtn, I18n::t(StringId::BACK_ARROW));

            TouchInput::Point tap;
            while (true) {
                if (TouchInput::wasTapped(tap)) break;
                delay(20);
            }

            if (languageBtn.contains(tap.x, tap.y)) {
                LanguageScreen::run(tft);
            } else if (unitsBtn.contains(tap.x, tap.y)) {
                UnitsScreen::run(tft);
            } else if (backupBtn.contains(tap.x, tap.y)) {
                bool ok = SettingsBackup::backup();
                showBriefMessage(tft, I18n::t(ok ? StringId::MENU_BACKUP_SAVED : StringId::MENU_BACKUP_FAILED),
                                 ok ? TFT_GREEN : TFT_RED);
            } else if (restoreBtn.contains(tap.x, tap.y)) {
                if (SettingsBackup::hasBackup()) {
                    bool ok = SettingsBackup::restore();
                    showBriefMessage(tft, I18n::t(ok ? StringId::MENU_RESTORED : StringId::MENU_RESTORE_FAILED),
                                     ok ? TFT_GREEN : TFT_RED);
                }
            } else if (backBtn.contains(tap.x, tap.y)) {
                page = 1;
            }
        }
    }
}

}