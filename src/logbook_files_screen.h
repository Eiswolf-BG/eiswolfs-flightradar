#pragma once
#include <Arduino.h>
#include <TFT_eSPI.h>

namespace LogbookFilesScreen {
    // Blockierend: listet die Logbuch-Dateien (ein Tag pro Zeile, mit Anzahl
    // Eintraegen) auf. Kehrt zurueck, sobald "Back" angetippt wird.
    void run(TFT_eSPI& tft);
}