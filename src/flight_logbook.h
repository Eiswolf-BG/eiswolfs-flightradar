#pragma once
#include <Arduino.h>

// SD-Karten-Flugbuch: protokolliert jedes NEU gesichtete Flugzeug (Zeit,
// Hex-Code, Rufzeichen, Kennzeichen, Typ, Distanz, Hoehe) in eine taegliche
// CSV-Datei auf der SD-Karte. Kann in den Einstellungen ("Flight Logbook")
// an-/ausgeschaltet werden. Ueberlebt einen Neustart am selben Tag, ohne
// bereits geloggte Flugzeuge erneut einzutragen (rekonstruiert die Liste
// beim Start aus der heutigen CSV-Datei).
namespace FlightLogbook {

    // Einmalig beim Boot aufrufen, NACHDEM die lokale Uhrzeit (UTC-Offset)
    // bekannt ist - liest die heutige CSV-Datei (falls vorhanden), um bereits
    // geloggte Flugzeuge nicht doppelt einzutragen.
    void init();

    // Periodisch aufrufen (z.B. nach jeder erfolgreichen ADS-B-Abfrage):
    // prueft auf neue, bisher ungesehene Flugzeuge und schreibt sie ins
    // heutige Logbuch. Kuemmert sich auch um den Tageswechsel (neue Datei,
    // neue "gesehen"-Liste).
    void update();

    // Anzahl der HEUTE bereits geloggten (unterschiedlichen) Flugzeuge.
    uint16_t todayCount();

    // Summiert alle taeglichen Logbuch-Dateien auf der SD-Karte:
    // 'totalAircraft' = Summe aller Eintraege ueber alle Tage,
    // 'totalDays' = Anzahl der Tage, an denen ueberhaupt geloggt wurde.
    void computeAllTimeStats(uint32_t& totalAircraft, uint16_t& totalDays);
}