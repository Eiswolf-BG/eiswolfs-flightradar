#pragma once
#include <Arduino.h>
struct Aircraft {
    char     hex[7]      = {0};
    char     callsign[9] = {0};
    char     reg[9]      = {0};
    char     typeCode[5] = {0};
    char     squawk[5]   = {0};
    char     category[3] = {0};

    float    lat            = 0;
    float    lon            = 0;
    int32_t  altBaroFt      = 0;
    int16_t  vertRateFtMin  = 0;
    float    groundSpeedKt  = 0;
    float    headingDeg     = 0;

    float    distanceKm     = 0;
    float    bearingDeg     = 0;

    uint32_t lastSeenMs     = 0;
    bool     alerted        = false;
    uint32_t alertedAtMs    = 0;

    bool     valid          = false;

    char     airlineName[24] = {0};
    uint16_t estSeats         = 0;
};