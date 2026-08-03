#include "wifi_manager.h"
#include "sd_storage.h"
#include <WiFi.h>
#include <SD.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <cstring>

namespace WifiMgr {

namespace {
    struct NetworkEntry {
        char ssid[33] = {0};
        char pass[64] = {0};
    };

    NetworkEntry networks[Config::MAX_WIFI_NETWORKS];
    uint8_t networkCountVal = 0;

    State state = State::Idle;
    uint32_t connectStartMs = 0;
    constexpr uint32_t CONNECT_TIMEOUT_MS = 15000;
    char ipStr[16] = {0};

    uint32_t lastReconnectAttemptMs = 0;
    constexpr uint32_t RECONNECT_RETRY_MS = 10000;

    SemaphoreHandle_t mutex = nullptr;

    void setState(State s) {
        xSemaphoreTake(mutex, portMAX_DELAY);
        state = s;
        xSemaphoreGive(mutex);
    }

    void loadFromSd() {
        networkCountVal = 0;
        if (!SdStorage::isMounted()) return;
        if (!SD.exists(Config::SD_WIFI_CREDENTIALS_FILE)) return;

        File f = SD.open(Config::SD_WIFI_CREDENTIALS_FILE, FILE_READ);
        if (!f) return;

        while (networkCountVal < Config::MAX_WIFI_NETWORKS && f.available()) {
            String ssid = f.readStringUntil('\n');
            String pass = f.readStringUntil('\n');
            ssid.trim();
            pass.trim();
            if (ssid.length() == 0) break;

            strncpy(networks[networkCountVal].ssid, ssid.c_str(), sizeof(networks[networkCountVal].ssid) - 1);
            strncpy(networks[networkCountVal].pass, pass.c_str(), sizeof(networks[networkCountVal].pass) - 1);
            networkCountVal++;
        }
        f.close();
    }

    void saveToSd() {
        if (!SdStorage::isMounted()) return;

        File f = SD.open(Config::SD_WIFI_CREDENTIALS_FILE, FILE_WRITE);
        if (!f) return;
        for (uint8_t i = 0; i < networkCountVal; i++) {
            f.println(networks[i].ssid);
            f.println(networks[i].pass);
        }
        f.close();
    }
}

void init() {
    if (mutex == nullptr) mutex = xSemaphoreCreateMutex();
    loadFromSd();
    setState(networkCountVal > 0 ? State::Idle : State::NoCredentials);
}

uint8_t networkCount() { return networkCountVal; }

String networkSsid(uint8_t index) {
    if (index >= networkCountVal) return String();
    return String(networks[index].ssid);
}

bool addNetwork(const char* ssid, const char* password) {
    if (networkCountVal >= Config::MAX_WIFI_NETWORKS) return false;

    strncpy(networks[networkCountVal].ssid, ssid, sizeof(networks[networkCountVal].ssid) - 1);
    strncpy(networks[networkCountVal].pass, password, sizeof(networks[networkCountVal].pass) - 1);
    networkCountVal++;
    saveToSd();
    return true;
}

void removeNetwork(uint8_t index) {
    if (index >= networkCountVal) return;
    for (uint8_t i = index; i < networkCountVal - 1; i++) {
        networks[i] = networks[i + 1];
    }
    networkCountVal--;
    networks[networkCountVal] = NetworkEntry{};
    saveToSd();
}

void connectTo(const char* ssid, const char* password) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    connectStartMs = millis();
    setState(State::Connecting);
}

void beginConnect() {
    if (networkCountVal == 0) {
        setState(State::NoCredentials);
        return;
    }

    int visibleCount = WiFi.scanNetworks(/*async=*/false);

    int8_t chosen = -1;
    for (uint8_t i = 0; i < networkCountVal && chosen < 0; i++) {
        for (int j = 0; j < visibleCount; j++) {
            if (WiFi.SSID(j) == networks[i].ssid) {
                chosen = (int8_t)i;
                break;
            }
        }
    }

    if (chosen < 0) {
        setState(State::Failed);
        return;
    }

    connectTo(networks[chosen].ssid, networks[chosen].pass);
}

void update() {
    State s = getState();

    if (s == State::Connecting) {
        if (WiFi.status() == WL_CONNECTED) {
            strncpy(ipStr, WiFi.localIP().toString().c_str(), sizeof(ipStr) - 1);
            setState(State::Connected);
            return;
        }
        if (millis() - connectStartMs > CONNECT_TIMEOUT_MS) {
            WiFi.disconnect(true);
            setState(State::Failed);
        }
        return;
    }

    if (s == State::Connected && WiFi.status() != WL_CONNECTED) {
        Serial.println("[WifiMgr] Verbindung verloren, versuche automatisch neu zu verbinden...");
        setState(State::Idle);
        lastReconnectAttemptMs = millis() - RECONNECT_RETRY_MS;
        return;
    }

    if ((s == State::Idle || s == State::Failed) && networkCountVal > 0) {
        if (millis() - lastReconnectAttemptMs >= RECONNECT_RETRY_MS) {
            lastReconnectAttemptMs = millis();
            beginConnect();
        }
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

}