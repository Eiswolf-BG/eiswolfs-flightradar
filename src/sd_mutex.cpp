#include "sd_mutex.h"
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

namespace SdMutex {

namespace {
    SemaphoreHandle_t mutex = nullptr;
}

void init() {
    if (mutex == nullptr) {
        mutex = xSemaphoreCreateRecursiveMutex();
    }
}

void lock() {
    if (mutex == nullptr) init();
    xSemaphoreTakeRecursive(mutex, portMAX_DELAY);
}

void unlock() {
    if (mutex != nullptr) {
        xSemaphoreGiveRecursive(mutex);
    }
}

}