#include "TwallGame.h"

Preferences preferences;

// --- Farbkonfiguration ---
// Diese Farben kannst du hier nach Belieben anpassen (Voraussetzung: Deine Farb-Makros wie PURPLE etc. existieren).
const uint32_t COLOR_SINGLE_PLAYER = GREEN;    // Single Player (für IDLE und im Spiel)
const uint32_t COLOR_MULTIPLAYER_IDLE = BLUE;  // Multiplayer (nur für den Start-Button im IDLE)
const uint32_t COLOR_P1 = MAGENTA;             // Player 1 (im Spiel und Gewinner-Screen)
const uint32_t COLOR_P2 = ORANGE;              // Player 2 (im Spiel und Gewinner-Screen)

// --- Variablen für die State Machine initialisieren ---
GameState gameState = IDLE;
GameMode currentMode = SINGLE_PLAYER;

uint8_t scoreP1 = 0;
uint8_t scoreP2 = 0;
uint8_t targetP1 = 99;
uint8_t targetP2 = 99;
uint8_t lastP1 = 99;
uint8_t lastP2 = 99;
uint32_t cooldownStartTime = 0;

uint32_t gameStartTime = 0;

#include <string.h> // Für strncpy

HighscoreEntry highscores[10];
int8_t lastNewHighscoreIndex = -1; // -1 bedeutet: aktuell kein neuer Highscore zur Benennung offen

void loadHighscores() {
    preferences.begin("game-data", false);
    
    // Prüfen, ob schon Daten existieren, sonst mit 0 und leeren Namen initialisieren
    size_t len = preferences.getBytesLength("top10_v2"); 
    if (len == sizeof(highscores)) {
        preferences.getBytes("top10_v2", &highscores, sizeof(highscores));
        Serial.println("Highscores geladen.");
    } else {
        Serial.println("Keine gültigen Highscores gefunden, initialisiere neu...");
        for (int i = 0; i < 10; i++) {
            highscores[i].score = 0;
            strncpy(highscores[i].name, "---", MAX_NAME_LEN);
        }
    }
    preferences.end();
    
    for(int i=0; i<10; i++) {
        Serial.printf("%d. %s: %d\n", i+1, highscores[i].name, highscores[i].score);
    }
}

void saveHighscores() {
    preferences.begin("game-data", false);
    preferences.putBytes("top10_v2", &highscores, sizeof(highscores)); // "top10_v2" damit wir das alte Format nicht crashen
    preferences.end();
}

void checkAndAddHighscore(uint8_t newScore) {
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
    strncpy(highscores[insertPos].name, "TrageDeinenNamenein!", MAX_NAME_LEN);
    
    // Merken, wo wir eingefügt haben, damit das Webinterface ihn später überschreiben kann
    lastNewHighscoreIndex = insertPos;
    
    saveHighscores();
    Serial.printf("Neuer Highscore auf Platz %d gespeichert!\n", insertPos + 1);
}

// Diese Funktion rufen wir später auf, wenn das Webinterface den Namen schickt
void updateLastHighscoreName(const char* newName) {
    if (lastNewHighscoreIndex >= 0 && lastNewHighscoreIndex < 10) {
        strncpy(highscores[lastNewHighscoreIndex].name, newName, MAX_NAME_LEN - 1);
        highscores[lastNewHighscoreIndex].name[MAX_NAME_LEN - 1] = '\0'; // Sicherheitshalber terminieren
        saveHighscores();
        Serial.printf("Name für Platz %d aktualisiert: %s\n", lastNewHighscoreIndex + 1, newName);
        
        // Optional: Reset des Index, falls man den Namen nur einmal ändern darf
        // lastNewHighscoreIndex = -1; 
    }
}

uint8_t getRandomGenerator(uint8_t min, uint8_t max){
    // Verhindert Division durch 0, falls min und max gleich sind 
    // (z.B. wenn jemand ein 1-Spalten-Spielfeld für 2 Spieler konfiguriert)
    if (min >= max) return min; 
    
    return esp_random() % (max-min) + min;
}

// --- 2. Angepasste Ziel-Generierung ---
void set_next_target(uint8_t player) {
    uint8_t nextField;
    uint8_t* currentTarget = (player == 1) ? &targetP1 : &targetP2;
    uint8_t* lastTarget = (player == 1) ? &lastP1 : &lastP2;
    uint32_t color;

    do {
        if (currentMode == SINGLE_PLAYER) {
            color = COLOR_SINGLE_PLAYER; // Dynamische Variable genutzt
            nextField = getRandomGenerator(0, NUM_FIELDS);
        } 
        else {
            color = (player == 1) ? COLOR_P1 : COLOR_P2; // Dynamische Variablen genutzt
            
            // 1. Beliebige Zeile auswürfeln (0 bis NUM_ROWS-1)
            uint8_t randRow = getRandomGenerator(0, NUM_ROWS); 
            uint8_t randCol;
            
            // 2. Spalte je nach Spieler auswürfeln
            if (player == 1) {
                // Player 1: Spalten von 0 bis zur Hälfte
                randCol = getRandomGenerator(0, NUM_COLUMNS / 2);
            } else {
                // Player 2: Spalten von der Hälfte bis zum Ende
                randCol = getRandomGenerator(NUM_COLUMNS / 2, NUM_COLUMNS);
            }
            
            // 3. 2D-Koordinaten wieder in 1D-Index umrechnen
            nextField = (randRow * NUM_COLUMNS) + randCol;
        }
        
    } while (nextField == *lastTarget);

    *currentTarget = nextField;
    *lastTarget = nextField;
    leds.set_rgb(color, *currentTarget);
}

void runGameLogic(uint32_t currentTime) {
    switch (gameState) {
        
        case IDLE:
            for (int i = 0; i < NUM_FIELDS; i++) leds.set_rgb(OFF, i);
            if ((currentTime / 500) % 2 == 0) {
                    leds.set_rgb(COLOR_SINGLE_PLAYER, 0);   // Taste für 1-Spieler
                    leds.set_rgb(COLOR_MULTIPLAYER_IDLE, 1); // Taste für 2-Spieler
                }

            // Startbedingung prüfen
            if (isPressed[0] || isPressed[1]) {
                // Variablen zurücksetzen
                scoreP1 = 0;
                scoreP2 = 0;
                lastP1 = 99; 
                lastP2 = 99;
                gameStartTime = currentTime;
                
                for (int i = 0; i < NUM_FIELDS; i++) leds.set_rgb(OFF, i);
                
                // Modus festlegen und Felder generieren
                if (isPressed[0]) {
                    currentMode = SINGLE_PLAYER;
                    set_next_target(1); 
                } else if (isPressed[1]) {
                    currentMode = MULTI_PLAYER;
                    set_next_target(1); // Start-Ziel für P1
                    set_next_target(2); // Start-Ziel für P2
                }
                
                gameState = PLAYING;
            }
            break;

        case PLAYING:
            //Vorzeitiger Abbruch, wenn Box 1 bis 4 (Indizes 0 bis 3) gleichzeitig gedrückt werden
            if (isPressed[0] && isPressed[1] && isPressed[2] && isPressed[3]) {
                Serial.println("Spiel vorzeitig abgebrochen!");
                gameState = GAME_OVER;
            }
            if (currentTime - gameStartTime >= 60000) {
                // Zeit abgelaufen!
                gameState = GAME_OVER;
            } else {
                // Player 1 Check (läuft im 1- und 2-Spieler-Modus)
                if (isPressed[targetP1]) {
                    scoreP1++;
                    leds.set_rgb(OFF, targetP1);
                    set_next_target(1);
                }
                
                // Player 2 Check (läuft NUR im 2-Spieler-Modus)
                if (currentMode == MULTI_PLAYER) {
                    if (isPressed[targetP2]) {
                        scoreP2++;
                        leds.set_rgb(OFF, targetP2);
                        set_next_target(2);
                    }
                }
            }
            break;

        case GAME_OVER:
            if (currentMode == SINGLE_PLAYER) {
                Serial.printf("Zeit abgelaufen! Score: %d\n", scoreP1);
                checkAndAddHighscore(scoreP1);
                
                // Das gesamte Feld in der Single Player Farbe leuchten lassen
                for (int i = 0; i < NUM_FIELDS; i++) {
                    leds.set_rgb(COLOR_SINGLE_PLAYER, i); 
                } 
            } else {
                Serial.printf("Zeit abgelaufen! P1: %d | P2: %d\n", scoreP1, scoreP2);
                checkAndAddHighscore(scoreP2);
                checkAndAddHighscore(scoreP1);
                
                // Multiplayer: Siegerfeld aufleuchten lassen
                for (int i = 0; i < NUM_FIELDS; i++) {
                    // Wir errechnen die Spalte anhand des 1D-Index i
                    // Annahme: Felder sind zeilenweise von links nach rechts durchnummeriert
                    uint8_t col = i % NUM_COLUMNS; 
                    
                    if (scoreP1 > scoreP2) {
                        // Player 1 hat gewonnen
                        if (col < NUM_COLUMNS / 2) {
                            leds.set_rgb(COLOR_P1, i); // Linke Hälfte
                        } else {
                            leds.set_rgb(OFF, i);      // Rechte Hälfte bleibt aus
                        }
                    } else if (scoreP2 > scoreP1) {
                        // Player 2 hat gewonnen
                        if (col >= NUM_COLUMNS / 2) {
                            leds.set_rgb(COLOR_P2, i); // Rechte Hälfte
                        } else {
                            leds.set_rgb(OFF, i);      // Linke Hälfte bleibt aus
                        }
                    } else {
                        // Unentschieden: Jeder behält seine Seite in seiner Farbe
                        if (col < NUM_COLUMNS / 2) {
                            leds.set_rgb(COLOR_P1, i); 
                        } else {
                            leds.set_rgb(COLOR_P2, i);
                        }
                    }
                }
            }
             
            cooldownStartTime = currentTime; // Startzeitpunkt für die 5 Sekunden Pause merken
            gameState = COOLDOWN;            // In den Cooldown-Status wechseln
            break;

        case COOLDOWN:
            if (currentTime - cooldownStartTime >= 5000) {
                // LEDs wieder ausschalten
                for (int i = 0; i < NUM_FIELDS; i++) leds.set_rgb(OFF, i);
                
                // Zurück in den Startbildschirm
                gameState = IDLE; 
            }
            break;
    }
}