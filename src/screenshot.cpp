#include "screenshot.h"
#include "config.h"
#include <SD.h>
#include <time.h>

namespace Screenshot {

namespace {
    void putU32(uint8_t* buf, uint32_t v) {
        buf[0] = v & 0xFF;
        buf[1] = (v >> 8) & 0xFF;
        buf[2] = (v >> 16) & 0xFF;
        buf[3] = (v >> 24) & 0xFF;
    }
}

String save(TFT_eSPI& tft) {
    if (!SD.exists(Config::SD_SCREENSHOT_DIR)) {
        SD.mkdir(Config::SD_SCREENSHOT_DIR);
    }

    time_t now = time(nullptr);
    struct tm tmNow;
    localtime_r(&now, &tmNow);
    char filename[32];
    snprintf(filename, sizeof(filename), "%04d%02d%02d_%02d%02d%02d.bmp",
             tmNow.tm_year + 1900, tmNow.tm_mon + 1, tmNow.tm_mday,
             tmNow.tm_hour, tmNow.tm_min, tmNow.tm_sec);

    char fullPath[64];
    snprintf(fullPath, sizeof(fullPath), "%s/%s", Config::SD_SCREENSHOT_DIR, filename);

    File f = SD.open(fullPath, FILE_WRITE);
    if (!f) return String();

    int32_t w = tft.width();
    int32_t h = tft.height();

    uint32_t rowSize = ((uint32_t)w * 3 + 3) & ~3u;
    uint32_t dataSize = rowSize * (uint32_t)h;
    uint32_t fileSize = 54 + dataSize;

    uint8_t header[54] = {0};
    header[0] = 'B'; header[1] = 'M';
    putU32(&header[2], fileSize);
    putU32(&header[10], 54);
    putU32(&header[14], 40);
    putU32(&header[18], (uint32_t)w);
    putU32(&header[22], (uint32_t)h);
    header[26] = 1;
    header[28] = 24;
    putU32(&header[34], dataSize);

    f.write(header, 54);

    uint16_t rowPixels[Config::SCREEN_WIDTH];
    uint8_t rowBuf[Config::SCREEN_WIDTH * 3 + 3];

    for (int32_t y = h - 1; y >= 0; y--) {
        tft.readRect(0, y, w, 1, rowPixels);
        for (int32_t x = 0; x < w; x++) {
            uint16_t px = rowPixels[x];
            uint8_t r5 = (px >> 11) & 0x1F;
            uint8_t g6 = (px >> 5) & 0x3F;
            uint8_t b5 = px & 0x1F;
            rowBuf[x * 3 + 0] = (b5 * 255) / 31;
            rowBuf[x * 3 + 1] = (g6 * 255) / 63;
            rowBuf[x * 3 + 2] = (r5 * 255) / 31;
        }
        for (uint32_t p = (uint32_t)w * 3; p < rowSize; p++) rowBuf[p] = 0;
        f.write(rowBuf, rowSize);
    }

    f.close();
    return String(filename);
}

}