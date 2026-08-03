#pragma once
#include <Arduino.h>

namespace SettingsBackup {
    bool backup();
    bool restore();
    bool hasBackup();
}