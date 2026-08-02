#pragma once
#include <Arduino.h>
struct Aircraft {
    char     hex[7]      = {0};
    char     callsign[9] = {0};
    char     reg[9]      = {0};
    char     typeCode[5] = {0};

    float    lat            = 0;
    float    lon            = 0;
    int32_t  altBaroFt      = 0;    // barometric altitude, ft
    int16_t  vertRateFtMin  = 0;    // vertical speed, ft/min (+climb / -descend)
    float    groundSpeedKt  = 0;
    float    headingDeg     = 0;    // track/heading

    float    distanceKm     = 0;    // computed relative to home, via Haversine
    float    bearingDeg     = 0;    // computed relative to home

    uint32_t lastSeenMs     = 0;    // millis() at last update, for stale-entry eviction
    bool     alerted        = false;// whether proximity beep already fired this pass
    uint32_t alertedAtMs    = 0;

    bool     valid          = false;

    char     airlineName[24] = {0}; // resolved from callsign ICAO prefix
    uint16_t estSeats         = 0;  // rough capacity estimate from typeCode, 0 = unknown
};