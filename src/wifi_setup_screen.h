#pragma once
#include <Arduino.h>
#include <TFT_eSPI.h>

namespace WifiSetupScreen {
    // Blockierend: WLAN-Netzwerke suchen, per Touch auswaehlen, Passwort
    // ueber Bildschirmtastatur eingeben (Klartext, keine Sterne), verbinden.
    // Speichert bei Erfolg die Zugangsdaten via WifiMgr auf der SD-Karte.
    // Rueckgabe: true = verbunden, false = abgebrochen/uebersprungen.
    bool run(TFT_eSPI& tft);
}