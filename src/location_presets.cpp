#include "location_presets.h"
#include "config.h"
#include "sd_mutex.h"
#include "sd_storage.h"
#include <SD.h>

namespace LocationPresets {

namespace {
    constexpr const char* PRESETS_FILE = "/Flightradar_cyd/locations.txt";

    struct Preset {
        double lat = 0;
        double lon = 0;
    };

    Preset presets[MAX_PRESETS];
    uint8_t presetCount = 0;
    int8_t activeIdx = -1;

    void saveToSd() {
        if (!SdStorage::isMounted()) return;
        SdMutex::Guard guard;

        File f = SD.open(PRESETS_FILE, FILE_WRITE);
        if (!f) return;
        f.printf("active=%d\n", (int)activeIdx);
        for (uint8_t i = 0; i < presetCount; i++) {
            f.printf("%.6f,%.6f\n", presets[i].lat, presets[i].lon);
        }
        f.close();
    }

    void loadFromSd() {
        presetCount = 0;
        activeIdx = -1;
        if (!SdStorage::isMounted()) return;
        SdMutex::Guard guard;

        if (!SD.exists(PRESETS_FILE)) return;
        File f = SD.open(PRESETS_FILE, FILE_READ);
        if (!f) return;

        bool firstLine = true;
        while (f.available()) {
            String line = f.readStringUntil('\n');
            line.trim();
            if (line.length() == 0) continue;

            if (firstLine) {
                firstLine = false;
                if (line.startsWith("active=")) {
                    activeIdx = (int8_t)line.substring(7).toInt();
                    continue;
                }
            }

            int comma = line.indexOf(',');
            if (comma < 0) continue;
            if (presetCount >= MAX_PRESETS) break;

            presets[presetCount].lat = line.substring(0, comma).toDouble();
            presets[presetCount].lon = line.substring(comma + 1).toDouble();
            presetCount++;
        }
        f.close();

        if (activeIdx >= (int8_t)presetCount) activeIdx = -1;
    }
}

void init() {
    loadFromSd();
}

uint8_t count() { return presetCount; }

void getLatLon(uint8_t index, double& lat, double& lon) {
    if (index >= presetCount) return;
    lat = presets[index].lat;
    lon = presets[index].lon;
}

bool addPreset(double lat, double lon) {
    if (presetCount >= MAX_PRESETS) return false;
    presets[presetCount].lat = lat;
    presets[presetCount].lon = lon;
    presetCount++;
    saveToSd();
    return true;
}

void removePreset(uint8_t index) {
    if (index >= presetCount) return;
    for (uint8_t i = index; i < presetCount - 1; i++) {
        presets[i] = presets[i + 1];
    }
    presetCount--;
    presets[presetCount] = Preset{};

    if (activeIdx == (int8_t)index) {
        activeIdx = -1;
    } else if (activeIdx > (int8_t)index) {
        activeIdx--;
    }
    saveToSd();
}

int8_t activeIndex() { return activeIdx; }

void setActiveIndex(int8_t index) {
    if (index >= (int8_t)presetCount) return;
    activeIdx = index;
    saveToSd();
}

}