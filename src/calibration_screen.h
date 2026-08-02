#pragma once
#include <Arduino.h>
#include <TFT_eSPI.h>

namespace CalibrationScreen {
    // Blockierend: zeigt 4 Kreise (Ecken), wartet auf Antippen jeweils in
    // Reihenfolge, berechnet die Kalibrierung und speichert sie auf der
    // SD-Karte (via TouchInput::saveCalibration()).
    void run(TFT_eSPI& tft);
}