#pragma once

// Hintergrund-Task fuer WLAN-Status, Standortbestimmung und ADS-B-Abfragen.
// Laeuft auf Core 0, damit Core 1 (Rendering + Touch im main-Loop) nie durch
// Netzwerk-Wartezeiten (WLAN, HTTPS) blockiert wird.
namespace NetTask {
    // Startet den Hintergrund-Task. Muss erst NACH WifiMgr::init() und
    // LocationManager::init() aufgerufen werden.
    void begin();
}