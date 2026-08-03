#pragma once
#include <Arduino.h>

// Steuert die diskrete RGB-LED auf der Rueckseite des CYD (kein Lautsprecher
// vorhanden, daher ersetzt die LED den akustischen Alarm vom Cardputer-
// Projekt). Pins: Rot=GPIO4, Gruen=GPIO16, Blau=GPIO17, active-low (LOW = an).
namespace LedAlert {

    enum class Mode {
        Off,             // keine LED
        ProximityGreen,  // Flugzeug innerhalb des Naeherungsradius -> gruen blinkt
        EmergencyRed,    // Notfall-Squawk (7500/7600/7700) -> rot blinkt schneller, hat Vorrang
    };

    // Einmalig in setup() aufrufen.
    void begin();

    // Haeufig aufrufen (z.B. alle 80-100ms). Schaltet die LED passend zum
    // Modus an/aus (blinkend, volle Helligkeit) und gibt den aktuellen
    // Blink-Zustand zurueck (true = LED gerade an), damit der Radar-
    // Bildschirm z.B. den betroffenen Punkt synchron mitblinken lassen kann.
    bool update(Mode mode, uint32_t nowMs);
}