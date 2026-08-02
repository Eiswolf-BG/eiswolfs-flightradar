#include "aircraft_table.h"
#include "radar_math.h"
#include <algorithm>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

// WICHTIG: Diese Datei ruft absichtlich KEIN AirlineLookup::resolve() mehr auf!
// postFetchUpdate() wird vom NetTask auf Core 0 aufgerufen. AirlineLookup
// braucht SD-Kartenzugriff, und die SD-Karte wurde in setup() auf Core 1
// initialisiert - Zugriff von Core 0 aus fuehrte zu einem Haenger (Task
// Watchdog auf IDLE0). Die Aufloesung der Airline-Namen passiert deshalb
// jetzt in main.cpp/renderAircraftList() auf Core 1 (demselben Core, der
// die SD-Karte urspruenglich initialisiert hat).

namespace AircraftTable {

namespace {
    Aircraft table[Config::MAX_TRACKED_AIRCRAFT];
    constexpr uint32_t STALE_TIMEOUT_MS = Config::FETCH_INTERVAL_MS * 3; // ~24s
    SemaphoreHandle_t mutex = nullptr;
    uint32_t versionCounter = 0;
}

void lock() { xSemaphoreTake(mutex, portMAX_DELAY); }
void unlock() { xSemaphoreGive(mutex); }

uint32_t version() { return versionCounter; }

void init() {
    if (mutex == nullptr) mutex = xSemaphoreCreateMutex();
    for (auto& a : table) a = Aircraft{};
}

Aircraft* raw() { return table; }
uint8_t capacity() { return Config::MAX_TRACKED_AIRCRAFT; }

uint8_t validCount() {
    uint8_t n = 0;
    for (auto& a : table) if (a.valid) n++;
    return n;
}

void postFetchUpdate(double homeLat, double homeLon) {
    uint32_t now = millis();

    for (auto& a : table) {
        if (!a.valid) continue;

        if (now - a.lastSeenMs > STALE_TIMEOUT_MS) {
            a = Aircraft{}; // evict
            continue;
        }

        auto polar = RadarMath::toPolar(homeLat, homeLon, a.lat, a.lon);
        a.distanceKm = polar.distanceKm;
        a.bearingDeg = polar.bearingDeg;
    }
    std::sort(table, table + Config::MAX_TRACKED_AIRCRAFT,
              [](const Aircraft& a, const Aircraft& b) {
                  if (a.valid != b.valid) return a.valid > b.valid;
                  if (!a.valid) return false;
                  return a.distanceKm < b.distanceKm;
              });
    versionCounter++;
}

}