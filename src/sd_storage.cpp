#include "sd_storage.h"
#include "config.h"
#include <SD.h>
#include <SPI.h>
#include <time.h>

namespace SdStorage {

namespace {
    bool mounted = false;
    SPIClass sdSpi(HSPI);
    const char* kDefaultAirlinesCsv =
        "icao,name\n"
        "BAW,British Airways\n"
        "SAA,South African Airways\n"
        "CAW,Comair\n"
        "FLY,Safair (FlySafair)\n"
        "KLM,KLM Royal Dutch Airlines\n"
        "DLH,Lufthansa\n"
        "UAE,Emirates\n"
        "QTR,Qatar Airways\n"
        "ETH,Ethiopian Airlines\n"
        "AFR,Air France\n"
        "SWR,Swiss International\n"
        "BAW,British Airways\n"
        "MSR,EgyptAir\n"
        "KQA,Kenya Airways\n"
        "UAL,United Airlines\n"
        "DAL,Delta Air Lines\n"
        "AAL,American Airlines\n";

    const char* kDefaultAircraftTypesCsv =
        "type,seats\n"
        "A320,180\n"
        "A321,220\n"
        "A319,140\n"
        "A332,278\n"
        "A333,277\n"
        "A359,314\n"
        "A388,469\n"
        "B738,189\n"
        "B737,148\n"
        "B739,180\n"
        "B77W,365\n"
        "B788,242\n"
        "B789,296\n"
        "E190,100\n"
        "CRJ2,50\n"
        "CRJ9,90\n"
        "DH8D,78\n";

    bool ensureDir(const char* path) {
        if (SD.exists(path)) return true;
        return SD.mkdir(path);
    }

    void writeIfAbsent(const char* path, const char* contents) {
        if (SD.exists(path)) return;
        File f = SD.open(path, FILE_WRITE);
        if (!f) return;
        f.print(contents);
        f.close();
    }
}

bool init() {
    sdSpi.begin(Config::SD_SPI_CLK_PIN, Config::SD_SPI_MISO_PIN,
                Config::SD_SPI_MOSI_PIN, Config::SD_SPI_CS_PIN);
    mounted = SD.begin(Config::SD_SPI_CS_PIN, sdSpi, 4000000);
    if (!mounted) return false;

    ensureDir(Config::SD_ROOT_DIR);
    ensureDir(Config::SD_LOG_DIR);
    ensureDir(Config::SD_SCREENSHOT_DIR);
    return true;
}

bool isMounted() { return mounted; }

void seedDefaultDataFiles() {
    if (!mounted) return;
    writeIfAbsent(Config::SD_AIRLINES_CSV, kDefaultAirlinesCsv);
    writeIfAbsent(Config::SD_AIRCRAFT_TYPES_CSV, kDefaultAircraftTypesCsv);
}

void logEvent(const char* csvLine) {
    if (!mounted) return;

    time_t now = time(nullptr);
    struct tm tmNow;
    localtime_r(&now, &tmNow);
    char filename[64];
    snprintf(filename, sizeof(filename), "%s/%04d-%02d-%02d.csv",
             Config::SD_LOG_DIR, tmNow.tm_year + 1900, tmNow.tm_mon + 1, tmNow.tm_mday);

    File f = SD.open(filename, FILE_APPEND);
    if (!f) return;
    f.println(csvLine);
    f.close();
}

}