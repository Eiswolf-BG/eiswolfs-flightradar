#pragma once
#include <Arduino.h>
#include <TFT_eSPI.h>

namespace SplashScreen {
    // Zeichnet den Splash-Hintergrund (Titel + gruene Flugzeug-Silhouette auf
    // schwarzem Grund) und merkt sich den Startzeitpunkt. NICHT blockierend -
    // der Aufrufer kann danach normal weiter booten und Status-Zeilen
    // draufschreiben (siehe setStatusLine()).
    void begin(TFT_eSPI& tft);

    // Schreibt/ueberschreibt eine der (begrenzt vielen) Status-Zeilen unten
    // im Splash-Screen, z.B. "SD-Karte: OK" oder "WLAN verbunden".
    // slot: 0 = erste Zeile, 1 = zweite Zeile, usw.
    void setStatusLine(TFT_eSPI& tft, uint8_t slot, const String& text, uint16_t color = TFT_WHITE);

    // Blockiert, bis seit begin() mindestens MIN_DISPLAY_MS vergangen sind.
    // Direkt vor dem Verlassen des Splash-Screens aufrufen.
    void waitRemaining();
}