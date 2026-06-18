#pragma once

#include <Arduino.h>
#include <LittleFS.h>
#include <array>

// Maximale Länge für den Spielernamen (inkl. Null-Terminator)
#define MAX_NAME_LEN 16

#include <mutex>

struct HighscoreEntry {
    uint8_t score;
    char name[MAX_NAME_LEN];
};

class HighscoreManager {
public:
    HighscoreManager() = default;
    
    void load();
    void checkAndAdd(uint8_t newScore, bool isDefaultTime);
    void updateHighscoreName(const int index, const char* newName, bool isDefaultTime);
    
    std::array<HighscoreEntry, 10> getHighscoresCopy(bool isDefaultTime) const;
    
    void clearDynamicHighscores();
    void reset60sHighscore();

private:
    HighscoreEntry highscores60[10];
    HighscoreEntry highscoresDynamic[10];
    int8_t lastNewHighscoreIndex = -1;
    mutable std::recursive_mutex mtx;

    void save60();
};