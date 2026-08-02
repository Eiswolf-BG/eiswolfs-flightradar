#pragma once
#include <Arduino.h>
#include "aircraft.h"
#include "config.h"

namespace AdsbClient {

    struct FetchResult {
        bool     ok = false;
        uint16_t aircraftCount = 0;
        int      httpCode = 0;
    };
    FetchResult fetch(double homeLat, double homeLon, float radiusKm,
                       Aircraft* table, uint8_t tableCapacity);
    void primeTime();

}