#pragma once
#include <stdint.h>

// Sehr stark vereinfachte Weltkarte (Kontinent-Umrisse) als 64x32-Raster-
// Bitmap, generiert aus Natural-Earth-Daten (1:110m-Aufloesung, die groebste
// oeffentlich verfuegbare Stufe - genau richtig fuer ein kleines Display).
// Jede Zeile ist ein 64-Bit-Wert; Bit (63-i) gesetzt = Landflaeche an
// Spalte i. Equirektangulare Projektion: Zeile 0 = Nordpol, Zeile 31 =
// Suedpol, Spalte 0 = -180 Grad Laenge, Spalte 63 = +180 Grad Laenge.
namespace WorldMap {
    constexpr uint8_t GRID_W = 64;
    constexpr uint8_t GRID_H = 32;
    extern const uint64_t ROWS[GRID_H];
}