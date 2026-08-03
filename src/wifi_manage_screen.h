#pragma once
#include <Arduino.h>
#include <TFT_eSPI.h>

namespace WifiManageScreen {
    // Blockierend: zeigt die bis zu 3 gespeicherten WLAN-Netzwerke, erlaubt
    // Loeschen einzelner Eintraege und (falls noch Platz ist) das Hinzufuegen
    // eines neuen ueber den Scan+Passwort-Bildschirm. Kehrt zurueck, sobald
    // "Back" angetippt wird.
    void run(TFT_eSPI& tft);
}