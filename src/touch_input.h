#pragma once
#include <Arduino.h>

namespace TouchInput {

    struct Point {
        int16_t x = 0;
        int16_t y = 0;
        bool touched = false;
    };

    void begin();

    // Laedt calibration.txt von der SD-Karte. Gibt false zurueck, wenn keine
    // vorhanden oder ungueltig ist.
    bool loadCalibration();
    void setCalibration(int16_t rawXmin, int16_t rawXmax, int16_t rawYmin, int16_t rawYmax);
    void saveCalibration();
    bool hasCalibration();

    // Aktueller Touch-Zustand, roh (unkalibriert), z.B. fuer die
    // Kalibrierungs-Routine selbst.
    Point rawPoint();

    // Aktueller Touch-Zustand, auf Bildschirmkoordinaten (0..SCREEN_WIDTH-1 /
    // 0..SCREEN_HEIGHT-1) umgerechnet und geclampt.
    Point mappedPoint();

    // Liefert genau einmal pro physischer Beruehrung "true" (beim Loslassen),
    // mitsamt der zuletzt bekannten Position. Fuer Buttons/Tastatur gedacht.
    bool wasTapped(Point& outPoint);
}