#include "net_task.h"
#include "config.h"
#include "wifi_manager.h"
#include "location_manager.h"
#include "adsb_client.h"
#include "aircraft_table.h"
#include "aircraft.h"
#include "settings_store.h"
#include "aircraft_details.h"
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <cstring>

namespace NetTask {

namespace {
    TaskHandle_t taskHandle = nullptr;
    uint32_t lastFetchMs = 0;

    // Eigener Zwischenspeicher fuer die Netzwerk-Antwort. Die eigentliche
    // AircraftTable wird NUR fuer den kurzen Kopiervorgang gesperrt, NICHT
    // waehrend der (langsamen) Netzwerkabfrage selbst - sonst friert der
    // Radar-Bildschirm fuer die Dauer der HTTPS-Anfrage ein, weil er auf
    // denselben Lock wartet.
    Aircraft tempTable[Config::MAX_TRACKED_AIRCRAFT];

    void taskFunc(void*) {
        for (;;) {
            WifiMgr::update();
            LocationManager::update();

            // Erledigt eine evtl. anstehende Detail-Abfrage (Modell/Route)
            // fuer das aktuell vom Nutzer ausgewaehlte Flugzeug, falls es eine
            // gibt. Guenstig, wenn nichts ansteht (nur ein Mutex-Check).
            AircraftDetails::update();

            if (millis() - lastFetchMs >= Config::FETCH_INTERVAL_MS) {
                lastFetchMs = millis();

                if (WifiMgr::getState() == WifiMgr::State::Connected) {
                    LocationManager::requestIpLookupIfNeeded();

                    double lat = 0, lon = 0;
                    LocationManager::getHomeLocation(lat, lon);

                    float rangeKm = Config::RANGE_STEPS_KM[SettingsStore::rangeIndex()];

                    // Netzwerkabfrage OHNE Lock - schreibt nur in den lokalen
                    // Zwischenspeicher, den sonst niemand anfasst. Der
                    // Radar-Bildschirm kann waehrenddessen ganz normal mit den
                    // ALTEN Daten weiterzeichnen.
                    auto result = AdsbClient::fetch(lat, lon, rangeKm,
                                                     tempTable, Config::MAX_TRACKED_AIRCRAFT);

                    if (result.ok) {
                        AircraftTable::lock();
                        memcpy(AircraftTable::raw(), tempTable,
                               sizeof(Aircraft) * Config::MAX_TRACKED_AIRCRAFT);
                        AircraftTable::postFetchUpdate(lat, lon);
                        AircraftTable::unlock();
                    } else {
                        Serial.printf("[NetTask] Abfrage fehlgeschlagen (HTTP %d)\n", result.httpCode);
                    }
                }
            }

            vTaskDelay(pdMS_TO_TICKS(50));
        }
    }
}

void begin() {
    xTaskCreatePinnedToCore(
        taskFunc,
        "NetTask",
        20480,     // Stack: TLS-Handshake + JSON-Parsing braucht mehr als das Minimum
        nullptr,
        1,         // Prioritaet
        &taskHandle,
        0          // Core 0 (Core 1 bleibt frei fuer Rendering/Touch im main-Loop)
    );
}

}