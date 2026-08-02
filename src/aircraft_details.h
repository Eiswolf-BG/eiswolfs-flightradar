#pragma once
#include <Arduino.h>

// Additional aircraft details (model) that are NOT part of the ADS-B signal
// and get looked up via the free hexdb.io community database by hex code -
// only for the currently selected aircraft (not for all of them, to keep
// network load low).
namespace AircraftDetails {

    struct Info {
        bool loading = false;
        char model[40] = {0}; // e.g. "Airbus A320 216", empty if unknown
    };

    // Called from Core 1 (touch selection): marks that details should be
    // fetched for this aircraft (if not already done).
    void request(const char* hex);

    // Called from Core 1 to get the current (possibly still incomplete)
    // state for 'hex'.
    Info get(const char* hex);

    // Called periodically from NetTask (Core 0): performs a pending request
    // (blocking HTTPS call, but that's fine - runs in the background and
    // only briefly delays the next ADS-B poll).
    void update();
}