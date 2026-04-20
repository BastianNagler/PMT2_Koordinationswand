#include "TwallGame.h"

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
uint32_t lastBlinkTime = 0;
uint8_t help_blink = 0;

uint8_t getRandomGenerator(uint8_t min, uint8_t max){
    return esp_random() % (max-min) + min;
}
void set_next_target(uint8_t player) {
    uint8_t minLimit, maxLimit;
    uint8_t* currentTarget;
    uint8_t* lastTarget;
    uint32_t color;

    if (currentMode == SINGLE_PLAYER) {
        // Ein Spieler: Ganzes Spielfeld
        minLimit = 0; 
        maxLimit = NUM_FIELDS;
        currentTarget = &targetP1;
        lastTarget = &lastP1;
        color = CYAN;
    } else { // 2 SPIELER MODUS
        if (player == 1) {
            // Player 1: Erste Hälfte
            minLimit = 0; 
            maxLimit = NUM_FIELDS / 2;
            currentTarget = &targetP1;
            lastTarget = &lastP1;
            color = MAGENTA; // Eigene Farbe für P1
        } else {
            // Player 2: Zweite Hälfte
            minLimit = NUM_FIELDS / 2; 
            maxLimit = NUM_FIELDS;
            currentTarget = &targetP2;
            lastTarget = &lastP2;
            color = ORANGE; // Eigene Farbe für P2
        }
    }

    // Neues Feld auswürfeln (nicht das gleiche wie vorher)
   // do {
        *currentTarget = getRandomGenerator(minLimit, maxLimit);
    //} while (*currentTarget == *lastTarget);
    
    *lastTarget = *currentTarget;
    leds.set_rgb(color, *currentTarget);
}

void runGameLogic(uint32_t currentTime) {
    switch (gameState) {
        
        case IDLE:
            if (currentTime - lastBlinkTime >= 500) {
                lastBlinkTime = currentTime;
                for (int i = 0; i < NUM_FIELDS; i++) leds.set_rgb(OFF, i);
                
                if (help_blink == 1) {
                    leds.set_rgb(YELLOW, 0); // Taste für 1-Spieler
                    leds.set_rgb(BLUE, 1);   // Taste für 2-Spieler
                }
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
            }
            
            for (int i = 0; i < NUM_FIELDS; i++) leds.set_rgb(OFF, i); 
            
            lastBlinkTime = currentTime; 
            gameState = IDLE;            
            break;
    }
}
