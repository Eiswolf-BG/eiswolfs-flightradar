#pragma once
#include <Arduino.h>

namespace SettingsStore {
    void load();
    void save();

    uint8_t rangeIndex();
    void setRangeIndex(uint8_t idx);

    bool displayInverted();
    void setDisplayInverted(bool inverted);

    bool emergencyAlertEnabled();
    void setEmergencyAlertEnabled(bool on);

    bool proximityAlertEnabled();
    void setProximityAlertEnabled(bool on);

    bool flightLogbookEnabled();
    void setFlightLogbookEnabled(bool on);
}