#pragma once
#include <Arduino.h>

// Gemeinsames Lock fuer ALLE SD-Kartenzugriffe im gesamten Projekt. Die
// SD-Bibliothek ist nicht thread-sicher: ohne dieses Lock koennen
// gleichzeitige Zugriffe von NetTask (Core 0, z.B. Flugbuch-Eintrag
// schreiben alle 8s) und dem Haupt-Loop (Core 1, z.B. Einstellungen im
// Menue speichern) das Geraet einfrieren, wenn sie zeitlich zusammentreffen.
// Rekursiv, damit verschachtelte Aufrufe (z.B. FlightLogbook::update() ruft
// intern ensureCurrentDate() auf, das seinerseits auch sperrt) nicht
// blockieren.
namespace SdMutex {
    void init();
    void lock();
    void unlock();

    // RAII-Helfer: sperrt im Konstruktor, gibt im Destruktor frei - so kann
    // man das Lock nicht versehentlich vergessen freizugeben (z.B. bei
    // einem fruehen "return").
    class Guard {
    public:
        Guard() { lock(); }
        ~Guard() { unlock(); }
        Guard(const Guard&) = delete;
        Guard& operator=(const Guard&) = delete;
    };
}