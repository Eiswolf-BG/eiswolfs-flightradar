#pragma once
#include <Arduino.h>

namespace LocationManager {

    enum class Source { GpsFix, IpGeolocation, Manual, Persisted, None };

    void init();
    void update();
    void requestIpLookupIfNeeded();
    void getHomeLocation(double& lat, double& lon);

    Source currentSource();
    void setManualLocation(double lat, double lon);
    void setGpsEnabled(bool enabled);
    bool isGpsEnabled();

    void cycleGpsPinPair();
    const char* currentGpsPinLabel();

    bool hasGpsFix();

}