#include "airline_filter.h"
#include "config.h"
#include "sd_mutex.h"
#include "sd_storage.h"
#include <SD.h>
#include <cstring>
#include <cctype>

namespace AirlineFilter {

namespace {
    constexpr const char* HIDDEN_FILE = "/Flightradar_cyd/hidden_airlines.txt";

    char hidden[MAX_HIDDEN][4] = {{0}};
    uint8_t hiddenCount = 0;

    void extractPrefix(const char* callsign, char* out) {
        int i = 0;
        for (; i < 3 && callsign[i] && isalpha((unsigned char)callsign[i]); i++) {
            out[i] = (char)toupper((unsigned char)callsign[i]);
        }
        out[i] = '\0';
    }

    void saveToSd() {
        if (!SdStorage::isMounted()) return;
        SdMutex::Guard guard;

        File f = SD.open(HIDDEN_FILE, FILE_WRITE);
        if (!f) return;
        for (uint8_t i = 0; i < hiddenCount; i++) {
            f.println(hidden[i]);
        }
        f.close();
    }

    void loadFromSd() {
        hiddenCount = 0;
        if (!SdStorage::isMounted()) return;
        SdMutex::Guard guard;

        if (!SD.exists(HIDDEN_FILE)) return;
        File f = SD.open(HIDDEN_FILE, FILE_READ);
        if (!f) return;

        while (f.available() && hiddenCount < MAX_HIDDEN) {
            String line = f.readStringUntil('\n');
            line.trim();
            if (line.length() == 0) continue;
            strncpy(hidden[hiddenCount], line.c_str(), 3);
            hidden[hiddenCount][3] = 0;
            hiddenCount++;
        }
        f.close();
    }
}

void init() {
    loadFromSd();
}

uint8_t count() { return hiddenCount; }

String icaoAt(uint8_t index) {
    if (index >= hiddenCount) return String();
    return String(hidden[index]);
}

bool addHidden(const char* icaoPrefix) {
    if (hiddenCount >= MAX_HIDDEN) return false;
    if (!icaoPrefix || !icaoPrefix[0]) return false;

    char normalized[4] = {0};
    uint8_t i = 0;
    for (; i < 3 && icaoPrefix[i]; i++) {
        normalized[i] = (char)toupper((unsigned char)icaoPrefix[i]);
    }
    normalized[i] = 0;
    if (i == 0) return false;

    for (uint8_t j = 0; j < hiddenCount; j++) {
        if (strcmp(hidden[j], normalized) == 0) return true;
    }

    strncpy(hidden[hiddenCount], normalized, 3);
    hidden[hiddenCount][3] = 0;
    hiddenCount++;
    saveToSd();
    return true;
}

void removeHidden(uint8_t index) {
    if (index >= hiddenCount) return;
    for (uint8_t i = index; i < hiddenCount - 1; i++) {
        strncpy(hidden[i], hidden[i + 1], 4);
    }
    hiddenCount--;
    hidden[hiddenCount][0] = 0;
    saveToSd();
}

bool isHidden(const char* callsign) {
    if (hiddenCount == 0) return false;
    char prefix[4];
    extractPrefix(callsign, prefix);
    if (!prefix[0]) return false;

    for (uint8_t i = 0; i < hiddenCount; i++) {
        if (strcmp(hidden[i], prefix) == 0) return true;
    }
    return false;
}

}