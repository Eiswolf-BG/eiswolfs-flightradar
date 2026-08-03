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
#include "flight_logbook.h"

TFT_eSPI tft = TFT_eSPI();

constexpr int16_t CONTENT_TOP = 30; // schlanker Header, Radar bekommt den Rest des Screens
constexpr uint32_t POLL_INTERVAL_MS = 300; // wie oft wir NACHSCHAUEN, ob sich Daten geaendert haben
constexpr uint32_t SWEEP_TICK_MS = 80; // wie oft der Sweep-Strahl ein Stueck weiterdreht
uint32_t lastPollMs = 0;
uint32_t lastSweepMs = 0;
uint32_t lastRenderedVersion = 0xFFFFFFFF; // erzwingt einen ersten Render-Aufruf
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

void drawMenuButton() {
    tft.fillRoundRect(menuBtn.x, menuBtn.y, menuBtn.w, menuBtn.h, 4, TFT_NAVY);
    tft.drawRoundRect(menuBtn.x, menuBtn.y, menuBtn.w, menuBtn.h, 4, TFT_DARKGREY);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_WHITE, TFT_NAVY);
    tft.drawString("Menu", menuBtn.x + menuBtn.w / 2, menuBtn.y + menuBtn.h / 2);
    tft.setTextDatum(TL_DATUM);
}

void drawHeader() {
    // Bis CONTENT_TOP loeschen, damit kein Bildrest vom Menue zwischen Header
    // und Radar-Bildschirm haengen bleibt. Schlanker Header, damit der Radar-
    // Kreis darunter maximal viel Platz bekommt.
    tft.fillRect(0, 0, Config::SCREEN_WIDTH, CONTENT_TOP, TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(1);
    tft.setCursor(6, 10);
    tft.println("Eiswolfs Flightradar");
    drawMenuButton();
}

// Zeigt eine grosse, unmissverstaendliche Meldung und haelt das Geraet an -
// KEIN weiterer Bildschirm erscheint, solange keine SD-Karte steckt. Die App
// braucht die Karte fuer Einstellungen, WLAN-Zugangsdaten und Nachschlage-
// tabellen, daher macht ein Weiterlaufen ohne sie keinen Sinn.
void haltWithSdRequiredScreen() {
    tft.fillScreen(TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    int16_t cx = Config::SCREEN_WIDTH / 2;
    int16_t cy = Config::SCREEN_HEIGHT / 2;

    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.setTextSize(2);
    tft.drawString("For this app a", cx, cy - 40);
    tft.drawString("SD card is", cx, cy - 10);
    tft.drawString("required", cx, cy + 20);

    tft.setTextSize(1);
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.drawString("Insert a card and restart the device", cx, cy + 60);
    tft.setTextDatum(TL_DATUM);

    while (true) {
        delay(1000); // haengen bleiben - absichtlich kein weiterer Screen
    }
}

// Zeigt/versteckt ein blinkendes Notfall-Banner im Header-Bereich, wenn ein
// Flugzeug einen Notfall-Squawk (7500/7600/7700) sendet. Ersetzt kurzzeitig
// den normalen Titel; sobald der Notfall vorbei ist, wird drawHeader() genau
// einmal wieder aufgerufen, um den Titel sauber wiederherzustellen.
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
    }
}

void setup() {
    Serial.begin(115200);
    delay(300);

    tft.init();
    tft.setRotation(0);
    // Sofort loeschen, bevor irgendetwas anderes passiert (SD-Init etc.
    // braucht einen Moment) - sonst zeigt das Display kurz zufaelligen
    // Bildspeicher-Muell an, bevor der erste echte Screen gezeichnet wird.
    tft.fillScreen(TFT_BLACK);

    TouchInput::begin();
    LedAlert::begin();

    // --- SD-Karte: PFLICHT. Ohne Karte kein weiterer Screen. ---
    bool sdOk = SdStorage::init();
    if (!sdOk) {
        haltWithSdRequiredScreen();
        return; // unerreichbar (haltWithSdRequiredScreen() haengt fuer immer), nur zur Klarheit
    }
    SdStorage::seedDefaultDataFiles();

    // Einstellungen (u.a. Display-Invertierung) VOR dem Splash laden und
    // anwenden, damit der Splash selbst schon in der richtigen Ausrichtung
    // gezeichnet wird (kein Farbwechsel mitten in der Anzeige).
    SettingsStore::load();
    tft.invertDisplay(SettingsStore::displayInverted());

    WifiMgr::init(); // laedt die gespeicherten Netzwerke (bis zu 3) von der SD-Karte

    // --- Splash-Screen: schwarzer Hintergrund, gruenes Flugzeug, mind. 5s ---
    SplashScreen::begin(tft);
    SplashScreen::setStatusLine(tft, 0, "SD Card: OK", TFT_WHITE);

    // --- Touch-Kalibrierung (nur beim allerersten Start, oder wenn Datei fehlt) ---
    if (!TouchInput::loadCalibration()) {
        CalibrationScreen::run(tft);
    }

    // --- WLAN: Ersteinrichtung (falls noch kein Netzwerk gespeichert ist)
    //     oder automatisches Verbinden mit dem ersten gerade sichtbaren
    //     gespeicherten Netzwerk (z.B. Zuhause ODER Auto-Hotspot). ---
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

    // --- Standort per IP-Geolocation (einmalig blockierend beim Start) ---
    SplashScreen::setStatusLine(tft, 2, "Getting location...");
    LocationManager::init();
    uint32_t locStart = millis();
    while (LocationManager::currentSource() == LocationManager::Source::None &&
           millis() - locStart < 8000) {
        LocationManager::requestIpLookupIfNeeded();
        delay(200);
    }
    SplashScreen::setStatusLine(tft, 2, "Ready!");

    // Sobald die IP-Geolocation einen UTC-Offset geliefert hat, die
    // Zeitzone entsprechend setzen - Zeitstempel (Flugbuch, Log-Dateinamen)
    // zeigen dann die ECHTE Ortszeit statt UTC, automatisch weltweit richtig.
    if (LocationManager::hasUtcOffset()) {
        configTime(LocationManager::utcOffsetSeconds(), 0, "pool.ntp.org", "time.nist.gov");
    }

    AircraftTable::init();
    AirlineLookup::init();
    FlightLogbook::init(); // rekonstruiert die "heute schon geloggt"-Liste aus der SD-Karte

    // --- Ab hier uebernimmt der Netzwerk-Task (Core 0) laufend WLAN-Status,
    //     Standort-Updates und ADS-B-Abfragen im Hintergrund. ---
    NetTask::begin();

    // Splash bleibt mindestens 5 Sekunden sichtbar, egal wie schnell der Rest war.
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
            forceRedraw = true; // sofort neu zeichnen
        } else if (tap.y >= CONTENT_TOP) {
            if (RadarScreen::handleTap(tap.x, tap.y, CONTENT_TOP)) {
                forceRedraw = true; // sofort neu zeichnen nach Interaktion
            }
        }
    }

    // Nur alle POLL_INTERVAL_MS kurz nachschauen (billige Abfrage eines
    // Zaehlers), statt staendig teuer neu zu zeichnen. Ein echtes Neuzeichnen
    // (render()) passiert nur, wenn sich die Flugzeugdaten TATSAECHLICH
    // geaendert haben (neue Abfrage im Hintergrund fertig) oder der Nutzer
    // etwas angetippt hat - das vermeidet unnoetiges Flackern.
    if (forceRedraw || millis() - lastPollMs >= POLL_INTERVAL_MS) {
        lastPollMs = millis();
        uint32_t currentVersion = AircraftTable::version();
        if (forceRedraw || currentVersion != lastRenderedVersion) {
            lastRenderedVersion = currentVersion;
            forceRedraw = false;
            RadarScreen::render(tft, CONTENT_TOP);
            lastSweepMs = millis(); // Sweep-Delta nicht ueber den Vollbild-Redraw hinweg aufaddieren
        }
    }

    // Sweep-Strahl dreht sich unabhaengig von den Flugzeugdaten weiter -
    // billige Linien-Zeichnung ohne Vollbild-Clear, daher kein Flackern.
    uint32_t nowMs = millis();

    // Naeherungs-/Notfall-Alarm (LED) laeuft IMMER, auch wenn gerade das
    // Detail-Fenster offen ist - unabhaengig vom Sweep/Radar-Redraw.
    RadarScreen::updateProximityAlert(nowMs);

    if (nowMs - lastSweepMs >= SWEEP_TICK_MS) {
        uint32_t deltaMs = nowMs - lastSweepMs;
        lastSweepMs = nowMs;
        RadarScreen::tick(tft, CONTENT_TOP, deltaMs);
        updateEmergencyBanner(nowMs); // gleiche Taktung wie der Sweep-Tick
    }
}