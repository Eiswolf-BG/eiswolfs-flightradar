#pragma once

// Simple unit conversions for display purposes. Instead of trying to
// auto-detect the user's region (extra network call, error-prone), we just
// show BOTH metric and aviation/imperial units side by side, e.g.
// "9144m / 30000ft" - simpler, more robust, and useful to everyone
// regardless of where the CYD is actually used.
namespace Units {
    constexpr float FT_TO_M = 0.3048f;
    constexpr float KT_TO_KMH = 1.852f;
    constexpr float KM_TO_NM = 1.0f / 1.852f;

    inline float feetToMeters(float ft) { return ft * FT_TO_M; }
    inline float ktToKmh(float kt) { return kt * KT_TO_KMH; }
    inline float kmToNm(float km) { return km * KM_TO_NM; }
}