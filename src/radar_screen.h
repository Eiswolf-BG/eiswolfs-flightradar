#pragma once
#include <Arduino.h>
#include <TFT_eSPI.h>

namespace RadarScreen {
    // Zeichnet den kompletten Radar-Frame neu (Ringe, Flugzeuge als Punkte,
    // Rufzeichen-Label, Sweep-Strahl, Auswahl-Infoleiste, Reichweiten-Button)
    // im Bereich von y=top bis zum unteren Bildschirmrand. Wird nur bei
    // tatsaechlichen Datenaenderungen oder nach Touch-Interaktion aufgerufen
    // (siehe main.cpp), NICHT bei jedem Sweep-Tick - sonst wuerde es flackern.
    void render(TFT_eSPI& tft, int16_t top);

    // Guenstiger, haeufiger Aufruf (z.B. alle 100ms): dreht NUR den
    // Sweep-Strahl ein Stueck weiter und zeichnet ihn (plus dezente
    // Nachzieh-Linien), OHNE den Bildschirm vorher zu loeschen. So entsteht
    // eine fluessige Drehbewegung ohne Vollbild-Neuzeichnen (= kein Flackern).
    // deltaMs = vergangene Zeit seit dem letzten tick()-Aufruf.
    void tick(TFT_eSPI& tft, int16_t top, uint32_t deltaMs);

    // Verarbeitet einen Touch-Tap (Reichweite umschalten, Flugzeug auswaehlen,
    // leere Flaeche antippen = Auswahl aufheben). Gibt true zurueck, wenn der
    // Tap im Radar-Bereich (y >= top) verarbeitet wurde.
    bool handleTap(int16_t x, int16_t y, int16_t top);

    // Haeufig aufrufen (unabhaengig davon, ob ein Detail-Fenster offen ist):
    // prueft, ob ein Flugzeug innerhalb des Alarmradius ist ODER ein
    // Notfall-Squawk sendet, steuert die LED entsprechend (Notfall hat
    // Vorrang, blinkt rot statt gruen) und merkt sich den Blink-Zustand,
    // damit tick() den betroffenen Punkt synchron mitblinken lassen kann.
    void updateProximityAlert(uint32_t nowMs);

    struct EmergencyInfo {
        bool active = false;
        char callsign[9] = {0};
        char squawk[5] = {0};
    };

    // Reine Abfrage (kein Seiteneffekt): gibt das erste Flugzeug mit einem
    // Notfall-Squawk zurueck, falls vorhanden. Fuer das Banner im Header.
    EmergencyInfo checkEmergency();
}