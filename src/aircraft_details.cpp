#include "aircraft_details.h"
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <cstring>

namespace AircraftDetails {

namespace {
    SemaphoreHandle_t mutex = nullptr;

    char pendingHex[7] = {0};
    bool hasPending = false;

    char cachedHex[7] = {0};
    Info cached;

    void ensureMutex() {
        if (mutex == nullptr) mutex = xSemaphoreCreateMutex();
    }

    bool httpGetString(WiFiClientSecure& client, const String& url, String& outBody) {
        HTTPClient http;
        http.setTimeout(5000);
        if (!http.begin(client, url)) return false;
        int code = http.GET();
        bool ok = (code == HTTP_CODE_OK);
        if (ok) outBody = http.getString();
        http.end();
        return ok;
    }
}

void request(const char* hex) {
    ensureMutex();
    xSemaphoreTake(mutex, portMAX_DELAY);
    if (strcmp(cachedHex, hex) != 0 && strcmp(pendingHex, hex) != 0) {
        strncpy(pendingHex, hex, sizeof(pendingHex) - 1);
        hasPending = true;
    }
    xSemaphoreGive(mutex);
}

Info get(const char* hex) {
    ensureMutex();
    xSemaphoreTake(mutex, portMAX_DELAY);
    Info out;
    if (strcmp(cachedHex, hex) == 0) {
        out = cached;
    } else if (strcmp(pendingHex, hex) == 0 && hasPending) {
        out.loading = true;
    }
    xSemaphoreGive(mutex);
    return out;
}

void update() {
    ensureMutex();

    char hex[7] = {0};
    bool doWork = false;

    xSemaphoreTake(mutex, portMAX_DELAY);
    if (hasPending) {
        strncpy(hex, pendingHex, sizeof(hex) - 1);
        doWork = true;
    }
    xSemaphoreGive(mutex);

    if (!doWork) return;

    Info result;

    WiFiClientSecure client;
    client.setInsecure();
    client.setTimeout(5000);

    String body;
    if (httpGetString(client, String("https://hexdb.io/api/v1/aircraft/") + hex, body)) {
        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, body);
        if (!err) {
            const char* manufacturer = doc["Manufacturer"] | "";
            const char* type = doc["Type"] | "";
            if (manufacturer[0] && type[0]) {
                snprintf(result.model, sizeof(result.model), "%s %s", manufacturer, type);
            } else if (type[0]) {
                strncpy(result.model, type, sizeof(result.model) - 1);
            }
        }
    }

    xSemaphoreTake(mutex, portMAX_DELAY);
    strncpy(cachedHex, hex, sizeof(cachedHex) - 1);
    cached = result;
    hasPending = false;
    xSemaphoreGive(mutex);
}

}