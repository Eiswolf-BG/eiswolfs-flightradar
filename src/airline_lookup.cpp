#include "airline_lookup.h"
#include "sd_storage.h"
#include "config.h"
#include <SD.h>

namespace AirlineLookup {

namespace {
    constexpr uint8_t CACHE_SIZE = 16;

    struct AirlineCacheEntry {
        char icao[4] = {0};
        char name[24] = {0};
        bool used = false;
    };
    struct TypeCacheEntry {
        char type[5] = {0};
        uint16_t seats = 0;
        bool used = false;
    };

    AirlineCacheEntry airlineCache[CACHE_SIZE];
    TypeCacheEntry typeCache[CACHE_SIZE];

    uint8_t hash3(const char* s) {
        uint32_t h = 2166136261u;
        for (int i = 0; i < 3 && s[i]; i++) h = (h ^ s[i]) * 16777619u;
        return h % CACHE_SIZE;
    }

    bool lookupCsv(const char* path, const char* key, char* outValue, size_t outLen) {
        if (!SdStorage::isMounted()) return false;
        File f = SD.open(path);
        if (!f) return false;

        bool found = false;
        f.readStringUntil('\n');
        while (f.available()) {
            String line = f.readStringUntil('\n');
            int comma = line.indexOf(',');
            if (comma < 0) continue;
            String k = line.substring(0, comma);
            k.trim();
            if (k.equalsIgnoreCase(key)) {
                String v = line.substring(comma + 1);
                v.trim();
                strncpy(outValue, v.c_str(), outLen - 1);
                found = true;
                break;
            }
        }
        f.close();
        return found;
    }
    void extractAirlinePrefix(const char* callsign, char* out /* size 4 */) {
        int i = 0;
        for (; i < 3 && callsign[i] && isalpha((unsigned char)callsign[i]); i++) {
            out[i] = callsign[i];
        }
        out[i] = '\0';
    }
}

void init() {
    clearCache();
}

void clearCache() {
    memset(airlineCache, 0, sizeof(airlineCache));
    memset(typeCache, 0, sizeof(typeCache));
}

void resolve(Aircraft& a) {
    char prefix[4];
    extractAirlinePrefix(a.callsign, prefix);
    if (prefix[0]) {
        uint8_t slot = hash3(prefix);
        AirlineCacheEntry& c = airlineCache[slot];
        if (c.used && strncmp(c.icao, prefix, 3) == 0) {
            strncpy(a.airlineName, c.name, sizeof(a.airlineName) - 1);
        } else {
            char name[24] = {0};
            if (lookupCsv(Config::SD_AIRLINES_CSV, prefix, name, sizeof(name))) {
                strncpy(a.airlineName, name, sizeof(a.airlineName) - 1);
                strncpy(c.icao, prefix, 3);
                strncpy(c.name, name, sizeof(c.name) - 1);
                c.used = true;
            }
        }
    }

    if (a.typeCode[0]) {
        uint8_t slot = hash3(a.typeCode);
        TypeCacheEntry& c = typeCache[slot];
        if (c.used && strncmp(c.type, a.typeCode, 4) == 0) {
            a.estSeats = c.seats;
        } else {
            char seatsStr[8] = {0};
            if (lookupCsv(Config::SD_AIRCRAFT_TYPES_CSV, a.typeCode, seatsStr, sizeof(seatsStr))) {
                a.estSeats = atoi(seatsStr);
                strncpy(c.type, a.typeCode, 4);
                c.seats = a.estSeats;
                c.used = true;
            }
        }
    }
}

}