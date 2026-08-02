#pragma once
#include <Arduino.h>
#include <TFT_eSPI.h>

namespace MenuScreen {
    // Blockierend: einfaches Menue (aktuell nur "Touch kalibrieren").
    // Kehrt zurueck, sobald "Zurueck" angetippt wird.
    void run(TFT_eSPI& tft);
}