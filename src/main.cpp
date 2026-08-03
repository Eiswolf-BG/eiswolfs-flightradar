#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <SD.h>
#include <WiFi.h>
#include <time.h>

#include "config.h"
#include "aircraft.h"
#include "radar_math.h"
#include "aircraft_table.h"
#include "airline_lookup.h"
#include "sd_storage.h"
#include "wifi_manager.h"
#include "location_manager.h"
#include "adsb_client.h"
#include "touch_input.h"
#include "calibration_screen.h"
#include "wifi_setup_screen.h"
#include "menu_screen.h"
#include "settings_store.h"
#include "net_task.h"
#include "radar_screen.h"
#include "splash_screen.h"
#include "led_alert.h"
#include "flight_logbook.h"
#include "screenshot.h"

TFT_eSPI tft = TFT_eSPI();

// Schlanker Header (Titel + Buttons) PLUS eine zweite, duenne Statuszeile
// (Uhrzeit + WLAN-Signalstaerke) darunter - der Radar-Kreis bekommt den Rest
// des Screens.
constexpr int16_t HEADER_TITLE_H = 30;
constexpr int16_t STATUS_LINE_H = 12;
constexpr int16_t CONTENT_TOP = HEADER_TITLE_H + STATUS_LINE_H;
constexpr uint32_t POLL_INTERVAL_MS = 300;
constexpr uint32_t SWEEP_TICK_MS = 80;
constexpr uint32_t STATUS_LINE_UPDATE_MS = 1000;
uint32_t lastPollMs = 0;
uint32_t lastSweepMs = 0;
uint32_t lastStatusLineMs = 0;
uint32_t lastRenderedVersion = 0xFFFFFFFF;
bool forceRedraw = false;
bool wasEmergency = false;
bool bannerBlinkOn = false;

struct Rect {
    int16_t x, y, w, h;
    bool contains(int16_t px, int16_t py) const {
        return px >= x && px < x + w && py >= y && py < y + h;
    }
};

Rect menuBtn = {Config::SCREEN_WIDTH - 60, 3, 54, 22};
Rect camBtn = {(int16_t)(menuBtn.x - 46), 3, 42, 22};

void drawMenuButton() {
    tft.fillRoundRect(menuBtn.x, menuBtn.y, menuBtn.w, menuBtn.h, 4, TFT_NAVY);
    tft.drawRoundRect(menuBtn.x, menuBtn.y, menuBtn.w, menuBtn.h, 4, TFT_DARKGREY);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_WHITE, TFT_NAVY);
    tft.drawString("Menu", menuBtn.x + menuBtn.w / 2, menuBtn.y + menuBtn.h / 2);
    tft.setTextDatum(TL_DATUM);
}

void drawCamButton() {
    tft.fillRoundRect(camBtn.x, camBtn.y, camBtn.w, camBtn.h, 4, TFT_NAVY);
    tft.drawRoundRect(camBtn.x, camBtn.y, camBtn.w, camBtn.h, 4, TFT_DARKGREY);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_WHITE, TFT_NAVY);
    tft.drawString("Cam", camBtn.x + camBtn.w / 2, camBtn.y + camBtn.h / 2);
    tft.setTextDatum(TL_DATUM);
}

void drawHeader() {
    tft.fillRect(0, 0, Config::SCREEN_WIDTH, CONTENT_TOP, TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(1);
    tft.setCursor(6, 10);
    tft.println("Eiswolfs Flightradar");
    drawCamButton();
    drawMenuButton();
}

void updateStatusLine() {
    if (wasEmergency) return;

    tft.fillRect(0, HEADER_TITLE_H, Config::SCREEN_WIDTH, STATUS_LINE_H, TFT_BLACK);
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);

    time_t now = time(nullptr);
    if (now > 8 * 3600 * 2) {
        struct tm tmNow;
        localtime_r(&now, &tmNow);
        char timeBuf[6];
        snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d", tmNow.tm_hour, tmNow.tm_min);
        tft.setCursor(6, HEADER_TITLE_H + 2);
        tft.print(timeBuf);
    }

    if (WiFi.status() == WL_CONNECTED) {
        char rssiBuf[14];
        snprintf(rssiBuf, sizeof(rssiBuf), "WiFi %ddBm", WiFi.RSSI());
        tft.setTextDatum(TR_DATUM);
        tft.drawString(rssiBuf, Config::SCREEN_WIDTH - 4, HEADER_TITLE_H + 2);
        tft.setTextDatum(TL_DATUM);
    }
}

void takeScreenshotWithFeedback() {
    LedAlert::flashWhite();

    String filename = Screenshot::save(tft);

    tft.fillRect(0, 0, Config::SCREEN_WIDTH, CONTENT_TOP, TFT_NAVY);
    tft.setTextColor(TFT_WHITE, TFT_NAVY);
    tft.setCursor(6, 10);
    if (filename.length() > 0) {
        tft.print("Saved: " + filename);
    } else {
        tft.setTextColor(TFT_RED, TFT_NAVY);
        tft.print("Screenshot failed");
    }
    delay(1200);
    drawHeader();
    updateStatusLine();
}

void haltWithSdRequiredScreen() {
    tft.invertDisplay(true);

    tft.fillScreen(TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    int16_t cx = Config::SCREEN_WIDTH / 2;
    int16_t cy = Config::SCREEN_HEIGHT / 2;

    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.setTextSize(2);
    tft.drawString("For this app a", cx, cy - 40);
    tft.drawString("SD card is", cx, cy - 10);
    tft.drawString("required", cx, cy + 20);

    tft.setTextSize(1);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.drawString("Insert a card and restart the device", cx, cy + 60);
    tft.setTextDatum(TL_DATUM);

    while (true) {
        delay(1000);
    }
}

void updateEmergencyBanner(uint32_t nowMs) {
    RadarScreen::EmergencyInfo emergency = RadarScreen::checkEmergency();

    if (emergency.active) {
        bannerBlinkOn = !bannerBlinkOn;
        uint16_t bg = bannerBlinkOn ? TFT_RED : TFT_BLACK;
        tft.fillRect(0, 0, Config::SCREEN_WIDTH, CONTENT_TOP, bg);
        tft.setTextColor(TFT_WHITE, bg);
        tft.setTextSize(1);
        tft.setCursor(4, 10);
        tft.printf("EMERGENCY %s %s", emergency.squawk, emergency.callsign);
        wasEmergency = true;
    } else if (wasEmergency) {
        wasEmergency = false;
        drawHeader();
        updateStatusLine();
    }
}

void setup() {
    Serial.begin(115200);
    delay(300);

    tft.init();
    tft.setRotation(0);
    tft.fillScreen(TFT_BLACK);

    TouchInput::begin();
    LedAlert::begin();

    bool sdOk = SdStorage::init();
    if (!sdOk) {
        haltWithSdRequiredScreen();
        return;
    }
    SdStorage::seedDefaultDataFiles();

    SettingsStore::load();
    tft.invertDisplay(SettingsStore::displayInverted());

    WifiMgr::init();

    SplashScreen::begin(tft);
    SplashScreen::setStatusLine(tft, 0, "SD Card: OK", TFT_WHITE);

    if (!TouchInput::loadCalibration()) {
        CalibrationScreen::run(tft);
    }

    if (WifiMgr::networkCount() == 0) {
        WifiSetupScreen::run(tft);
    } else {
        SplashScreen::setStatusLine(tft, 1, "Connecting WiFi...");
        WifiMgr::beginConnect();

        uint32_t waitStart = millis();
        while (WifiMgr::getState() == WifiMgr::State::Connecting && millis() - waitStart < 16000) {
            WifiMgr::update();
            delay(100);
        }

        if (WifiMgr::getState() == WifiMgr::State::Connected) {
            SplashScreen::setStatusLine(tft, 1, String("WiFi OK: ") + WifiMgr::getIP());
        } else {
            SplashScreen::setStatusLine(tft, 1, "WiFi FAILED", TFT_RED);
        }
    }

    AdsbClient::primeTime();

    SplashScreen::setStatusLine(tft, 2, "Getting location...");
    LocationManager::init();
    uint32_t locStart = millis();
    while (LocationManager::currentSource() == LocationManager::Source::None &&
           millis() - locStart < 8000) {
        LocationManager::requestIpLookupIfNeeded();
        delay(200);
    }
    SplashScreen::setStatusLine(tft, 2, "Ready!");

    if (LocationManager::hasUtcOffset()) {
        configTime(LocationManager::utcOffsetSeconds(), 0, "pool.ntp.org", "time.nist.gov");
    }

    AircraftTable::init();
    AirlineLookup::init();
    FlightLogbook::init();

    NetTask::begin();

    SplashScreen::waitRemaining();

    drawHeader();
    updateStatusLine();
    RadarScreen::render(tft, CONTENT_TOP);
}

void loop() {
    TouchInput::Point tap;
    if (TouchInput::wasTapped(tap)) {
        if (menuBtn.contains(tap.x, tap.y)) {
            MenuScreen::run(tft);
            drawHeader();
            updateStatusLine();
            forceRedraw = true;
        } else if (camBtn.contains(tap.x, tap.y)) {
            takeScreenshotWithFeedback();
        } else if (tap.y >= CONTENT_TOP) {
            if (RadarScreen::handleTap(tap.x, tap.y, CONTENT_TOP)) {
                forceRedraw = true;
            }
        }
    }

    if (forceRedraw || millis() - lastPollMs >= POLL_INTERVAL_MS) {
        lastPollMs = millis();
        uint32_t currentVersion = AircraftTable::version();
        if (forceRedraw || currentVersion != lastRenderedVersion) {
            lastRenderedVersion = currentVersion;
            forceRedraw = false;
            RadarScreen::render(tft, CONTENT_TOP);
            lastSweepMs = millis();
        }
    }

    uint32_t nowMs = millis();

    RadarScreen::updateProximityAlert(nowMs);

    if (nowMs - lastSweepMs >= SWEEP_TICK_MS) {
        uint32_t deltaMs = nowMs - lastSweepMs;
        lastSweepMs = nowMs;
        RadarScreen::tick(tft, CONTENT_TOP, deltaMs);
        updateEmergencyBanner(nowMs);
    }

    if (nowMs - lastStatusLineMs >= STATUS_LINE_UPDATE_MS) {
        lastStatusLineMs = nowMs;
        updateStatusLine();
    }
}