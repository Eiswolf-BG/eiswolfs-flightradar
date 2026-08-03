#pragma once
#include <Arduino.h>
#include <TFT_eSPI.h>

namespace StatsScreen {
    // Blockierend: zeigt einfache Statistiken aus dem Flight Logbook
    // (heute gesehen, insgesamt gesehen, Anzahl Tage geloggt). Kehrt zurueck,
    // sobald "Back" angetippt wird.
    void run(TFT_eSPI& tft);
}