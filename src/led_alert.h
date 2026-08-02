#pragma once
#include <Arduino.h>

// Steuert die diskrete RGB-LED auf der Rueckseite des CYD (kein Lautsprecher
// vorhanden, daher ersetzt die LED den akustischen Naeherungsalarm vom
// Cardputer-Projekt). Pins: Rot=GPIO4, Gruen=GPIO16, Blau=GPIO17,
// active-low (LOW = an).
namespace LedAlert {

    // Einmalig in setup() aufrufen.
    void begin();

    // Haeufig aufrufen (z.B. alle 80-100ms). 'active' = mindestens ein
    // Flugzeug ist innerhalb des Alarmradius. Schaltet die gruene LED
    // entsprechend an/aus (blinkend, volle Helligkeit) und gibt den
    // aktuellen Blink-Zustand zurueck (true = LED gerade an), damit der
    // Radar-Bildschirm den betroffenen Punkt synchron mitblinken lassen kann.
    bool update(bool active, uint32_t nowMs);
}