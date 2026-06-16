#pragma once
#include <Arduino.h>

// Initialisiert WLAN, Dateisystem, Webserver und WebSockets
void initWebInterface();

// Räumt getrennte Clients aus dem RAM (verhindert Memory-Leaks)
void cleanupWebInterface();

// Benachrichtigt das Tablet, dass das Spiel gestartet ist (inkl. Modus)
void notifyGameStart(const char* mode);

// Sendet den aktuellen Punktestand an das Tablet
void updateScore(uint8_t scoreP1, uint8_t scoreP2);

// Sagt dem Tablet, dass das Spiel vorbei ist
void notifyGameOver();