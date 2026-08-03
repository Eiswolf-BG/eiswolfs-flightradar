#pragma once
#include <Arduino.h>

// SD-Karten-Flugbuch: protokolliert jedes NEU gesichtete Flugzeug (Zeit,
// Hex-Code, Rufzeichen, Kennzeichen, Typ, Distanz, Hoehe) in eine taegliche
// CSV-Datei auf der SD-Karte. Kann in den Einstellungen ("Flight Logbook")
// an-/ausgeschaltet werden. Ueberlebt einen Neustart am selben Tag, ohne
// bereits geloggte Flugzeuge erneut einzutragen (rekonstruiert die Liste
// beim Start aus der heutigen CSV-Datei).
namespace FlightLogbook {

    void init();
    void update();

    uint16_t todayCount();

    void computeAllTimeStats(uint32_t& totalAircraft, uint16_t& totalDays);

    struct DayEntry {
        char date[11] = {0}; // "YYYY-MM-DD"
        uint32_t count = 0;
    };

    // Fuellt 'out' mit bis zu 'maxEntries' Tagen (Dateiname + Anzahl
    // Eintraege), sortiert wie sie auf der SD-Karte liegen (i.d.R.
    // chronologisch). Gibt die tatsaechliche Anzahl gefuellter Eintraege
    // zurueck.
    uint8_t listDays(DayEntry* out, uint8_t maxEntries);
}