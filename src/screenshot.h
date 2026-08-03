#pragma once
#include <Arduino.h>
#include <TFT_eSPI.h>

// Liest den aktuell angezeigten Bildschirminhalt per SPI-Readback zurueck
// (TFT_MISO ist am CYD verkabelt) und speichert ihn als 24-Bit-BMP-Datei auf
// der SD-Karte. Funktioniert mit allem, was gerade angezeigt wird (Radar,
// Detail-Fenster, Menue, ...).
namespace Screenshot {
    // Blockierend (dauert ca. 1-2 Sekunden). Gibt bei Erfolg den Dateinamen
    // (ohne Pfad) zurueck, sonst einen leeren String.
    String save(TFT_eSPI& tft);
}