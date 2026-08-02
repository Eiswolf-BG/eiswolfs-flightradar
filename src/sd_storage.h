#pragma once
#include <Arduino.h>

namespace SdStorage {

    bool init();
    bool isMounted();
    void seedDefaultDataFiles();
    void logEvent(const char* csvLine);

}