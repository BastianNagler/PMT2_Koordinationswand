#include "HighscoreManager.h"
#include <string.h> // Für strncpy

const char* HIGHSCORE_PLACEHOLDER_NAME = "TrageDeinenNamenein!";

void HighscoreManager::load() {
    if (LittleFS.exists("/highscores.dat")) {
        File file = LittleFS.open("/highscores.dat", "r");
        if (file) {
            size_t bytesRead = file.read((uint8_t*)highscores, sizeof(highscores));
            file.close();
            if (bytesRead == sizeof(highscores)) {
                Serial.println("Highscores aus LittleFS geladen.");
            } else {
                Serial.println("Highscore-Datei unvollständig oder beschädigt. Initialisiere neu...");
                for (int i = 0; i < 10; i++) {
                    highscores[i].score = 0;
                    strlcpy(highscores[i].name, "---", MAX_NAME_LEN);
                }
            }
        }
    } else {
        Serial.println("Keine Highscore-Datei gefunden, initialisiere neu...");
        for (int i = 0; i < 10; i++) {
            highscores[i].score = 0;
            strlcpy(highscores[i].name, "---", MAX_NAME_LEN);
        }
    }
    
    for(int i=0; i<10; i++) {
        Serial.printf("%d. %s: %d\n", i+1, highscores[i].name, highscores[i].score);
    }
}

void HighscoreManager::save() {
    File file = LittleFS.open("/highscores.dat", "w");
    if (file) {
        file.write((uint8_t*)highscores, sizeof(highscores));
        file.close();
    } else {
        Serial.println("Fehler beim Speichern der Highscores in LittleFS.");
    }
}

void HighscoreManager::checkAndAdd(uint8_t newScore) {
    lastNewHighscoreIndex = -1; // Reset
    
    if (newScore <= highscores[9].score) return; // Nicht gut genug für Top 10

    // Platz finden
    int insertPos = 9;
    for (int i = 0; i < 10; i++) {
        if (newScore > highscores[i].score) {
            insertPos = i;
            break;
        }
    }

    // Alle Scores darunter um eins nach hinten schieben
    for (int i = 9; i > insertPos; i--) {
        highscores[i] = highscores[i-1];
    }

    // Neuen Score einfügen und Standardnamen setzen
    highscores[insertPos].score = newScore;
    strlcpy(highscores[insertPos].name, HIGHSCORE_PLACEHOLDER_NAME, MAX_NAME_LEN);
    
    // Merken, wo wir eingefügt haben, damit das Webinterface ihn später überschreiben kann
    lastNewHighscoreIndex = insertPos;
    
    save();
    Serial.printf("Neuer Highscore auf Platz %d gespeichert!\n", insertPos + 1);
}

void HighscoreManager::updateHighscoreName(const int index, const char *newName)
{

    strlcpy(highscores[index].name, newName, MAX_NAME_LEN);
    save();
    Serial.printf("Name für Platz %d aktualisiert: %s\n", index + 1, newName);
}