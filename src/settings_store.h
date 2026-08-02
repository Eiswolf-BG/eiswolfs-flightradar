#pragma once
#include <Arduino.h>

// Einfacher key=value Einstellungs-Speicher in /Flightradar_cyd/config.txt.
// Neue Einstellungen koennen spaeter einfach als weitere getX()/setX()-Paare
// dazu kommen, ohne das Dateiformat zu aendern (unbekannte Zeilen werden
// beim Laden ignoriert).
namespace SettingsStore {

    // Liest config.txt von der SD-Karte (falls vorhanden). Werte, die nicht
    // in der Datei stehen, behalten ihren Standardwert.
    void load();

    // Schreibt die aktuellen Werte zurueck auf die SD-Karte.
    void save();

    uint8_t rangeIndex();
    void setRangeIndex(uint8_t idx);

    bool displayInverted();
    void setDisplayInverted(bool inverted);
}