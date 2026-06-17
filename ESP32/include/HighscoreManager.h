#pragma once

#include <Arduino.h>
#include <LittleFS.h>

// Maximale Länge für den Spielernamen (inkl. Null-Terminator)
#define MAX_NAME_LEN 16

struct HighscoreEntry {
    uint8_t score;
    char name[MAX_NAME_LEN];
};

class HighscoreManager {
public:
    HighscoreManager() = default;
    
    void load();
    void checkAndAdd(uint8_t newScore);
    void updateHighscoreName(const int index, const char* newName);
    
    const HighscoreEntry* getHighscores() const { return highscores; }

private:
    HighscoreEntry highscores[10];
    int8_t lastNewHighscoreIndex = -1;

    void save();
};