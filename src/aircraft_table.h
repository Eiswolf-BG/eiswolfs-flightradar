#pragma once
#include "aircraft.h"
#include "config.h"

namespace AircraftTable {

    void init();

    Aircraft* raw(); 
    uint8_t capacity();
    uint8_t validCount();
    void postFetchUpdate(double homeLat, double homeLon);

    // Wird bei jedem postFetchUpdate() erhoeht. Damit koennen andere Teile des
    // Programms (z.B. der Render-Loop) erkennen, ob sich die Daten seit dem
    // letzten Mal ueberhaupt geaendert haben, statt stumpf auf Zeit zu pollen -
    // das vermeidet unnoetiges (und flackerndes) Neuzeichnen.
    uint32_t version();

    // Schuetzt den Zugriff auf raw()/validCount()/postFetchUpdate() zwischen
    // dem Netzwerk-Task (Core 0, schreibt) und dem Render-Loop (Core 1, liest).
    // Aufrufer muss lock() vor und unlock() nach jedem Zugriff aufrufen.
    void lock();
    void unlock();

}