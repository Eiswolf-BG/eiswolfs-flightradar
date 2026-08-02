#include "settings_store.h"
#include "config.h"
#include <SD.h>

namespace SettingsStore {

namespace {
    uint8_t rangeIdx = Config::DEFAULT_RANGE_INDEX;
    bool inverted = false;

    void applyKeyValue(const String& key, const String& value) {
        if (key == "range_index") {
            int v = value.toInt();
            if (v >= 0 && v < Config::RANGE_STEP_COUNT) {
                rangeIdx = (uint8_t)v;
            }
        } else if (key == "invert") {
            inverted = (value.toInt() != 0);
        }
        // Weitere Einstellungen hier ergaenzen (unbekannte Keys werden
        // einfach ignoriert, damit alte config.txt-Dateien kompatibel bleiben).
    }
}

void load() {
    if (!SD.exists(Config::SD_SETTINGS_FILE)) return;

    File f = SD.open(Config::SD_SETTINGS_FILE, FILE_READ);
    if (!f) return;

    while (f.available()) {
        String line = f.readStringUntil('\n');
        line.trim();
        if (line.length() == 0 || line.startsWith("#")) continue;

        int eq = line.indexOf('=');
        if (eq < 0) continue;

        String key = line.substring(0, eq);
        String value = line.substring(eq + 1);
        key.trim();
        value.trim();
        applyKeyValue(key, value);
    }
    f.close();
}

void save() {
    File f = SD.open(Config::SD_SETTINGS_FILE, FILE_WRITE);
    if (!f) return;
    f.printf("range_index=%d\n", rangeIdx);
    f.printf("invert=%d\n", inverted ? 1 : 0);
    f.close();
}

uint8_t rangeIndex() { return rangeIdx; }

void setRangeIndex(uint8_t idx) {
    if (idx < Config::RANGE_STEP_COUNT) {
        rangeIdx = idx;
        save();
    }
}

bool displayInverted() { return inverted; }

void setDisplayInverted(bool inv) {
    inverted = inv;
    save();
}

}