#include "TwallGame.h"

Preferences preferences;
uint8_t highscores[10] = {0,0,0,0,0,0,0,0,0,0};

// --- Variablen für die State Machine initialisieren ---
GameState gameState = IDLE;
GameMode currentMode = SINGLE_PLAYER;

uint8_t scoreP1 = 0;
uint8_t scoreP2 = 0;
uint8_t targetP1 = 99;
uint8_t targetP2 = 99;
uint8_t lastP1 = 99;
uint8_t lastP2 = 99;

uint32_t gameStartTime = 0;

void loadHighscores() {
    preferences.begin("game-data", false); // Ordner "game-data" öffnen
    // Versuche das Array zu lesen. Wenn nicht vorhanden, bleibt es bei 0.
    preferences.getBytes("top10", highscores, 10);
    preferences.end();
    
    Serial.println("Highscores geladen:");
    for(int i=0; i<10; i++) Serial.printf("%d. %d\n", i+1, highscores[i]);
}

void saveHighscores() {
    preferences.begin("game-data", false);
    preferences.putBytes("top10", highscores, 10);
    preferences.end();
}

void checkAndAddHighscore(uint8_t newScore) {
    if (newScore <= highscores[9]) return; // Nicht gut genug für Top 10

    // Platz finden
    int insertPos = 9;
    for (int i = 0; i < 10; i++) {
        if (newScore > highscores[i]) {
            insertPos = i;
            break;
        }
    }

    // Alle Scores darunter um eins nach hinten schieben
    for (int i = 9; i > insertPos; i--) {
        highscores[i] = highscores[i-1];
    }

    // Neuen Score einfügen
    highscores[insertPos] = newScore;
    
    // Speichern
    saveHighscores();
    Serial.println("Neuer Highscore gespeichert!");
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
            color = CYAN;
            nextField = getRandomGenerator(0, NUM_FIELDS);
        } 
        else {
            color = (player == 1) ? MAGENTA : ORANGE;
            
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
                    leds.set_rgb(YELLOW, 0); // Taste für 1-Spieler
                    leds.set_rgb(BLUE, 1);   // Taste für 2-Spieler
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

            } else {
                Serial.printf("Zeit abgelaufen! P1: %d | P2: %d\n", scoreP1, scoreP2);
                checkAndAddHighscore(scoreP2);
            }
            checkAndAddHighscore(scoreP1);
            for (int i = 0; i < NUM_FIELDS; i++) leds.set_rgb(OFF, i); 
             
            gameState = IDLE;            
            break;
    }
}
