#include "HighscoreManager.h"
#include <string.h> // Für strncpy
#include "WebLog.h"

const char* HIGHSCORE_PLACEHOLDER_NAME = "Insert Name";

void HighscoreManager::load() {
    if (LittleFS.exists("/highscores.dat")) {
        File file = LittleFS.open("/highscores.dat", "r");
        if (file) {
            size_t bytesRead = file.read((uint8_t*)highscores60, sizeof(highscores60));
            file.close();
            if (bytesRead == sizeof(highscores60)) {
                WebLog.println("Highscores aus LittleFS geladen.");
            } else {
                WebLog.println("Highscore-Datei unvollständig oder beschädigt. Initialisiere neu...");
                for (int i = 0; i < 10; i++) {
                    highscores60[i].score = 0;
                    strlcpy(highscores60[i].name, "---", MAX_NAME_LEN);
                }
            }
        }
    } else {
        WebLog.println("Keine Highscore-Datei gefunden, initialisiere neu...");
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
    for (int i = 0; i < 10; i++) {
        highscoresDynamic[i].score = 0;
        strlcpy(highscoresDynamic[i].name, "---", MAX_NAME_LEN);
    }
}

void HighscoreManager::reset60sHighscore() {
    for (int i = 0; i < 10; i++) {
        highscores60[i].score = 0;
        strlcpy(highscores60[i].name, "---", MAX_NAME_LEN);
    }
    save60();
    WebLog.println("60s-Highscore wurde erfolgreich zurückgesetzt.");
}

void HighscoreManager::save60() {
    File file = LittleFS.open("/highscores.dat", "w");
    if (file) {
        file.write((uint8_t*)highscores60, sizeof(highscores60));
        file.close();
    } else {
        WebLog.println("Fehler beim Speichern der Highscores in LittleFS.");
    }
}

void HighscoreManager::checkAndAdd(uint8_t newScore, bool isDefaultTime) {
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
    WebLog.printf("Neuer Highscore auf Platz %d gespeichert!\n", insertPos + 1);
}

void HighscoreManager::updateHighscoreName(const int index, const char *newName, bool isDefaultTime)
{
    HighscoreEntry* targetList = isDefaultTime ? highscores60 : highscoresDynamic;
    strlcpy(targetList[index].name, newName, MAX_NAME_LEN);
    if (isDefaultTime) {
        save60();
    }
    WebLog.printf("Name für Platz %d aktualisiert: %s\n", index + 1, newName);
}