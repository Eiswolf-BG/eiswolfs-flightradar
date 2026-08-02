#include "wifi_manager.h"
#include "config.h"
#include "sd_storage.h"
#include <WiFi.h>
#include <Preferences.h>
#include <SD.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

namespace WifiMgr {

namespace {
    Preferences prefs;
    State state = State::Idle;
    uint32_t connectStartMs = 0;
    constexpr uint32_t CONNECT_TIMEOUT_MS = 15000; // bounded, same fix as the Kinect sketch
    char ipStr[16] = {0};

    // Schuetzt 'state': der Netzwerk-Task (Core 0) schreibt es in
    // beginConnect()/update(), der Render-Loop (Core 1) liest es ueber
    // getState(). ipStr wird nur einmal beim Verbinden geschrieben und danach
    // nur gelesen, daher hier ohne eigenen Lock.
    SemaphoreHandle_t mutex = nullptr;

    void setState(State s) {
        xSemaphoreTake(mutex, portMAX_DELAY);
        state = s;
        xSemaphoreGive(mutex);
    }
}

void init() {
    if (mutex == nullptr) mutex = xSemaphoreCreateMutex();
    prefs.begin("adsb_radar", /*readOnly=*/false);
    setState(hasStoredCredentials() ? State::Idle : State::NoCredentials);
}

bool hasStoredCredentials() {
    String ssid = prefs.getString("ssid", "");
    return ssid.length() > 0;
}

void saveCredentials(const char* ssid, const char* password) {
    prefs.putString("ssid", ssid);
    prefs.putString("pass", password);
}

void beginConnect() {
    if (!hasStoredCredentials()) {
        setState(State::NoCredentials);
        return;
    }
    String ssid = prefs.getString("ssid", "");
    String pass = prefs.getString("pass", "");

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), pass.c_str());
    connectStartMs = millis();
    setState(State::Connecting);
}

void update() {
    if (getState() != State::Connecting) return;

    if (WiFi.status() == WL_CONNECTED) {
        strncpy(ipStr, WiFi.localIP().toString().c_str(), sizeof(ipStr) - 1);
        setState(State::Connected);
        return;
    }

    if (millis() - connectStartMs > CONNECT_TIMEOUT_MS) {
        WiFi.disconnect(true);
        setState(State::Failed);
    }
}

State getState() {
    xSemaphoreTake(mutex, portMAX_DELAY);
    State s = state;
    xSemaphoreGive(mutex);
    return s;
}

void beginScan() {
    WiFi.scanNetworks(/*async=*/true);
}

bool isScanComplete() {
    return WiFi.scanComplete() >= 0;
}

int getScanResultCount() {
    int n = WiFi.scanComplete();
    return n > 0 ? n : 0;
}

String getScanResultSSID(int index) {
    return WiFi.SSID(index);
}

int32_t getScanResultRSSI(int index) {
    return WiFi.RSSI(index);
}

const char* getIP() { return ipStr; }

bool loadCredentialsFromSd() {
    if (!SdStorage::isMounted()) return false;
    if (!SD.exists(Config::SD_WIFI_CREDENTIALS_FILE)) return false;

    File f = SD.open(Config::SD_WIFI_CREDENTIALS_FILE, FILE_READ);
    if (!f) return false;

    String ssid = f.readStringUntil('\n');
    String pass = f.readStringUntil('\n');
    f.close();

    ssid.trim();
    pass.trim();
    if (ssid.length() == 0) return false;

    saveCredentials(ssid.c_str(), pass.c_str());
    return true;
}

void saveCredentialsToSdIfMounted() {
    if (!SdStorage::isMounted()) return;

    String ssid = prefs.getString("ssid", "");
    String pass = prefs.getString("pass", "");
    if (ssid.length() == 0) return;

    File f = SD.open(Config::SD_WIFI_CREDENTIALS_FILE, FILE_WRITE);
    if (!f) return;
    f.println(ssid);
    f.println(pass);
    f.close();
}

}