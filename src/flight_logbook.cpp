#include "flight_logbook.h"
#include "config.h"
#include "aircraft.h"
#include "aircraft_table.h"
#include "settings_store.h"
#include <SD.h>
#include <time.h>
#include <cstring>

namespace FlightLogbook {

namespace {
    constexpr uint16_t MAX_SEEN = 400;
    char seenHex[MAX_SEEN][7];
    uint16_t seenCount = 0;
    char currentDateStr[11] = {0};

    bool computeDateStr(char* out, size_t outSize) {
        time_t now = time(nullptr);
        if (now < 8 * 3600 * 2) return false;
        struct tm tmNow;
        localtime_r(&now, &tmNow);
        snprintf(out, outSize, "%04d-%02d-%02d", tmNow.tm_year + 1900, tmNow.tm_mon + 1, tmNow.tm_mday);
        return true;
    }

    void logFilename(char* out, size_t outSize) {
        snprintf(out, outSize, "%s/%s.csv", Config::SD_LOG_DIR, currentDateStr);
    }

    bool alreadySeen(const char* hex) {
        for (uint16_t i = 0; i < seenCount; i++) {
            if (strcmp(seenHex[i], hex) == 0) return true;
        }
        return false;
    }

    void markSeen(const char* hex) {
        if (seenCount >= MAX_SEEN) return;
        strncpy(seenHex[seenCount], hex, sizeof(seenHex[seenCount]) - 1);
        seenHex[seenCount][sizeof(seenHex[seenCount]) - 1] = 0;
        seenCount++;
    }

    void loadSeenFromTodayFile() {
        seenCount = 0;
        char filename[64];
        logFilename(filename, sizeof(filename));
        if (!SD.exists(filename)) return;

        File f = SD.open(filename, FILE_READ);
        if (!f) return;

        bool firstLine = true;
        while (f.available() && seenCount < MAX_SEEN) {
            String line = f.readStringUntil('\n');
            line.trim();
            if (line.length() == 0) continue;
            if (firstLine) { firstLine = false; continue; }

            int firstComma = line.indexOf(',');
            if (firstComma < 0) continue;
            int secondComma = line.indexOf(',', firstComma + 1);
            String hex = (secondComma < 0) ? line.substring(firstComma + 1)
                                            : line.substring(firstComma + 1, secondComma);
            hex.trim();
            if (hex.length() > 0) markSeen(hex.c_str());
        }
        f.close();
    }

    void ensureCurrentDate() {
        char today[11];
        if (!computeDateStr(today, sizeof(today))) return;

        if (strcmp(today, currentDateStr) != 0) {
            strncpy(currentDateStr, today, sizeof(currentDateStr) - 1);
            loadSeenFromTodayFile();
        }
    }

    void writeLogLine(const Aircraft& a) {
        char filename[64];
        logFilename(filename, sizeof(filename));

        bool needsHeader = !SD.exists(filename);

        File f = SD.open(filename, FILE_APPEND);
        if (!f) return;

        if (needsHeader) {
            f.println("timestamp,hex,callsign,reg,type,distance_km,altitude_ft");
        }

        time_t now = time(nullptr);
        struct tm tmNow;
        localtime_r(&now, &tmNow);
        char timestamp[20];
        snprintf(timestamp, sizeof(timestamp), "%04d-%02d-%02d %02d:%02d:%02d",
                 tmNow.tm_year + 1900, tmNow.tm_mon + 1, tmNow.tm_mday,
                 tmNow.tm_hour, tmNow.tm_min, tmNow.tm_sec);

        f.printf("%s,%s,%s,%s,%s,%.1f,%d\n",
                 timestamp,
                 a.hex,
                 a.callsign[0] ? a.callsign : "",
                 a.reg[0] ? a.reg : "",
                 a.typeCode[0] ? a.typeCode : "",
                 a.distanceKm,
                 (int)a.altBaroFt);
        f.close();
    }
}

void init() {
    ensureCurrentDate();
}

void update() {
    if (!SettingsStore::flightLogbookEnabled()) return;

    ensureCurrentDate();
    if (currentDateStr[0] == 0) return;

    static Aircraft snapshot[Config::MAX_TRACKED_AIRCRAFT];
    uint8_t count = 0;

    AircraftTable::lock();
    Aircraft* table = AircraftTable::raw();
    for (uint8_t i = 0; i < AircraftTable::capacity(); i++) {
        if (table[i].valid) snapshot[count++] = table[i];
    }
    AircraftTable::unlock();

    for (uint8_t i = 0; i < count; i++) {
        if (!snapshot[i].hex[0]) continue;
        if (alreadySeen(snapshot[i].hex)) continue;
        markSeen(snapshot[i].hex);
        writeLogLine(snapshot[i]);
    }
}

uint16_t todayCount() { return seenCount; }

void computeAllTimeStats(uint32_t& totalAircraft, uint16_t& totalDays) {
    totalAircraft = 0;
    totalDays = 0;

    File dir = SD.open(Config::SD_LOG_DIR);
    if (!dir || !dir.isDirectory()) return;

    File entry = dir.openNextFile();
    while (entry) {
        if (!entry.isDirectory()) {
            String name = String(entry.name());
            if (name.endsWith(".csv")) {
                totalDays++;
                uint32_t lines = 0;
                while (entry.available()) {
                    entry.readStringUntil('\n');
                    lines++;
                }
                if (lines > 0) totalAircraft += (lines - 1);
            }
        }
        entry.close();
        entry = dir.openNextFile();
    }
    dir.close();
}

}