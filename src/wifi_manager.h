#pragma once
#include <Arduino.h>
#include "config.h"

namespace WifiMgr {
    enum class State {
        Idle, Connecting, Connected, Failed, NoCredentials
    };

    void init();
    void beginConnect();
    void connectTo(const char* ssid, const char* password);

    void update();
    State getState();
    const char* getIP();

    uint8_t networkCount();
    String networkSsid(uint8_t index);
    bool addNetwork(const char* ssid, const char* password);
    void removeNetwork(uint8_t index);

    void beginScan();
    bool isScanComplete();
    int  getScanResultCount();
    String getScanResultSSID(int index);
    int32_t getScanResultRSSI(int index);
}