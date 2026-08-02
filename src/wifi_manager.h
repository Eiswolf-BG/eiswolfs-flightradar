#pragma once
#include <Arduino.h>

namespace WifiMgr {
    enum class State {
        Idle, Connecting, Connected, Failed, NoCredentials
    };

    void init();
    void beginConnect();
    void update();
    State getState();

    void saveCredentials(const char* ssid, const char* password);
    bool hasStoredCredentials();

    bool loadCredentialsFromSd();
    void saveCredentialsToSdIfMounted();

    void beginScan();
    bool isScanComplete();
    int  getScanResultCount();
    String getScanResultSSID(int index);
    int32_t getScanResultRSSI(int index);

    const char* getIP();
}