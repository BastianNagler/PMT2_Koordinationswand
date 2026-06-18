#include "HighscoreManager.h"
#include <string.h> // Für strncpy
#include "WebLog.h"

const char* HIGHSCORE_PLACEHOLDER_NAME = "Insert Name";

void HighscoreManager::load() {
    std::lock_guard<std::recursive_mutex> lock(mtx);
    if (LittleFS.exists("/highscores.dat")) {
        File file = LittleFS.open("/highscores.dat", "r");
        if (file) {
            size_t bytesRead = file.read((uint8_t*)highscores60, sizeof(highscores60));
            file.close();
            if (bytesRead == sizeof(highscores60)) {
                WebLog.println("[HIGHSCORE] Loaded highscores from LittleFS.");
            } else {
                WebLog.println("[HIGHSCORE] Highscore-file corrputed, reinitialize...");
                for (int i = 0; i < 10; i++) {
                    highscores60[i].score = 0;
                    strlcpy(highscores60[i].name, "---", MAX_NAME_LEN);
                }
            }
        }
    } else {
        WebLog.println("[HIGHSCORE] No highscore-file found, reinitialize...");
        for (int i = 0; i < 10; i++) {
            highscores60[i].score = 0;
            strlcpy(highscores60[i].name, "---", MAX_NAME_LEN);
        }
    }
    
    clearDynamicHighscores();

    for(int i=0; i<10; i++) {
        WebLog.printf("%d. %s: %d\n", i+1, highscores60[i].name, highscores60[i].score);
    }
}

void HighscoreManager::clearDynamicHighscores() {
    std::lock_guard<std::recursive_mutex> lock(mtx);
    for (int i = 0; i < 10; i++) {
        highscoresDynamic[i].score = 0;
        strlcpy(highscoresDynamic[i].name, "---", MAX_NAME_LEN);
    }
}

void HighscoreManager::reset60sHighscore() {
    std::lock_guard<std::recursive_mutex> lock(mtx);
    for (int i = 0; i < 10; i++) {
        highscores60[i].score = 0;
        strlcpy(highscores60[i].name, "---", MAX_NAME_LEN);
    }
    save60();
    WebLog.println("[HIGHSCORE] Successfully reseted 60s-highscores.");
}

void HighscoreManager::save60() {
    std::lock_guard<std::recursive_mutex> lock(mtx);
    File file = LittleFS.open("/highscores.dat", "w");
    if (file) {
        file.write((uint8_t*)highscores60, sizeof(highscores60));
        file.close();
    } else {
        WebLog.println("[ERROR] Error when saving highscores in LittleFS.");
    }
}

void HighscoreManager::checkAndAdd(uint8_t newScore, bool isDefaultTime) {
    std::lock_guard<std::recursive_mutex> lock(mtx);
    lastNewHighscoreIndex = -1; // Reset
    HighscoreEntry* targetList = isDefaultTime ? highscores60 : highscoresDynamic;
    
    if (newScore <= targetList[9].score) return; // Nicht gut genug für Top 10

    // Platz finden
    int insertPos = 9;
    for (int i = 0; i < 10; i++) {
        if (newScore > targetList[i].score) {
            insertPos = i;
            break;
        }
    }

    // Alle Scores darunter um eins nach hinten schieben
    for (int i = 9; i > insertPos; i--) {
        targetList[i] = targetList[i-1];
    }

    // Neuen Score einfügen und Standardnamen setzen
    targetList[insertPos].score = newScore;
    strlcpy(targetList[insertPos].name, HIGHSCORE_PLACEHOLDER_NAME, MAX_NAME_LEN);
    
    // Merken, wo wir eingefügt haben, damit das Webinterface ihn später überschreiben kann
    lastNewHighscoreIndex = insertPos;
    
    if (isDefaultTime) {
        save60();
    }
    WebLog.printf("[HIGHSCORE] Saved new highscore at rank %d!\n", insertPos + 1);
}

void HighscoreManager::updateHighscoreName(const int index, const char *newName, bool isDefaultTime)
{
    std::lock_guard<std::recursive_mutex> lock(mtx);
    if (index < 0 || index >= 10) {
        WebLog.printf("[ERROR] Highscore name update failed: index %d out of bounds!\n", index);
        return;
    }
    if (newName == nullptr) {
        WebLog.println("[ERROR] Highscore name update failed: newName is null!");
        return;
    }
    HighscoreEntry* targetList = isDefaultTime ? highscores60 : highscoresDynamic;
    strlcpy(targetList[index].name, newName, MAX_NAME_LEN);
    if (isDefaultTime) {
        save60();
    }
    WebLog.printf("[HIGHSCORE] refreshed name for rank %d: %s\n", index + 1, newName);
}

std::array<HighscoreEntry, 10> HighscoreManager::getHighscoresCopy(bool isDefaultTime) const {
    std::lock_guard<std::recursive_mutex> lock(mtx);
    std::array<HighscoreEntry, 10> copy;
    const HighscoreEntry* source = isDefaultTime ? highscores60 : highscoresDynamic;
    for (int i = 0; i < 10; i++) {
        copy[i] = source[i];
    }
    return copy;
}