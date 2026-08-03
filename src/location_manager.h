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

    // UTC-Offset in Sekunden (inkl. evtl. Sommerzeit), ermittelt bei der
    // IP-Geolocation-Abfrage. 0/false, falls noch nicht bekannt.
    bool hasUtcOffset();
    int32_t utcOffsetSeconds();

    // Ob die Region (per IP-Geolocation-Laendercode) metrische Einheiten
    // nutzt (Meter/km) statt Fuss/Meilen. Default true (metrisch), bis die
    // IP-Abfrage etwas anderes ermittelt hat - nur die USA nutzen aktuell
    // eine Ausnahme.
    bool useMetricUnits();
}