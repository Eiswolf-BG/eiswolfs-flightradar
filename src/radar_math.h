#pragma once
#include <cstdint>

namespace RadarMath {

    struct PolarCoord {
        float distanceKm;
        float bearingDeg; // 0-360, 0 = North, clockwise
    };

    struct ScreenPoint {
        int16_t x;
        int16_t y;
    };
    PolarCoord toPolar(double lat0, double lon0, double lat1, double lon1);

    ScreenPoint toScreen(const PolarCoord& polar, int16_t centerX, int16_t centerY,
                         int16_t radiusPx, float rangeKm);

}