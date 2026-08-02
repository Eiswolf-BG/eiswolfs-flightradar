#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <SD.h>

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

TFT_eSPI tft = TFT_eSPI();

constexpr int16_t CONTENT_TOP = 30;
constexpr uint32_t POLL_INTERVAL_MS = 300;
constexpr uint32_t SWEEP_TICK_MS = 80;
uint32_t lastPollMs = 0;
uint32_t lastSweepMs = 0;
uint32_t lastRenderedVersion = 0xFFFFFFFF;
bool forceRedraw = false;

struct Rect {
    int16_t x, y, w, h;
    bool contains(int16_t px, int16_t py) const {
        return px >= x && px < x + w && py >= y && py < y + h;
    }
};

Rect menuBtn = {Config::SCREEN_WIDTH - 38, 3, 32, 22};

void drawMenuButton() {
    tft.fillRoundRect(menuBtn.x, menuBtn.y, menuBtn.w, menuBtn.h, 4, TFT_NAVY);
    tft.drawRoundRect(menuBtn.x, menuBtn.y, menuBtn.w, menuBtn.h, 4, TFT_DARKGREY);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_WHITE, TFT_NAVY);
    tft.drawString("...", menuBtn.x + menuBtn.w / 2, menuBtn.y + menuBtn.h / 2);
    tft.setTextDatum(TL_DATUM);
}

void drawHeader() {
    tft.fillRect(0, 0, Config::SCREEN_WIDTH, CONTENT_TOP, TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(1);
    tft.setCursor(6, 10);
    tft.println("Eiswolfs Flightradar");
    drawMenuButton();
}

void setup() {
    Serial.begin(115200);
    delay(300);

    tft.init();
    tft.setRotation(0);

    TouchInput::begin();
    LedAlert::begin();

    bool sdOk = SdStorage::init();
    if (sdOk) {
        SdStorage::seedDefaultDataFiles();
    }

    SettingsStore::load();
    tft.invertDisplay(SettingsStore::displayInverted());

    WifiMgr::init();

    SplashScreen::begin(tft);
    SplashScreen::setStatusLine(tft, 0, sdOk ? "SD Card: OK" : "SD Card: ERROR",
                                sdOk ? TFT_WHITE : TFT_RED);

    if (!TouchInput::loadCalibration()) {
        CalibrationScreen::run(tft);
    }

    bool haveWifiFile = SD.exists(Config::SD_WIFI_CREDENTIALS_FILE);
    if (!haveWifiFile) {
        WifiSetupScreen::run(tft);
    } else {
        if (!WifiMgr::hasStoredCredentials()) {
            WifiMgr::loadCredentialsFromSd();
        }
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

    AircraftTable::init();
    AirlineLookup::init();

    NetTask::begin();

    SplashScreen::waitRemaining();

    drawHeader();
    RadarScreen::render(tft, CONTENT_TOP);
}

void loop() {
    TouchInput::Point tap;
    if (TouchInput::wasTapped(tap)) {
        if (menuBtn.contains(tap.x, tap.y)) {
            MenuScreen::run(tft);
            drawHeader();
            forceRedraw = true;
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

    // Naeherungsalarm (LED) laeuft IMMER, auch wenn gerade das Detail-Fenster
    // offen ist - unabhaengig vom Sweep/Radar-Redraw.
    RadarScreen::updateProximityAlert(nowMs);

    if (nowMs - lastSweepMs >= SWEEP_TICK_MS) {
        uint32_t deltaMs = nowMs - lastSweepMs;
        lastSweepMs = nowMs;
        RadarScreen::tick(tft, CONTENT_TOP, deltaMs);
    }
}