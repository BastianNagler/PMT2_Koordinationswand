#include "TwallGame.h"
#include <cmath> // Für fabs() in der Ripple-Animation

// --- Konstanten für das Spielverhalten ---
const uint32_t GAME_DURATION_MS = 60000;
const uint32_t COOLDOWN_DURATION_MS = 5000;
const uint32_t IDLE_BLINK_INTERVAL_MS = 500;
const uint32_t RIPPLE_ANIMATION_DURATION_MS = 300;
const uint8_t INVALID_TARGET = 99;
const char* HIGHSCORE_PLACEHOLDER_NAME = "TrageDeinenNamenein!";

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
uint8_t targetP1 = INVALID_TARGET;
uint8_t targetP2 = INVALID_TARGET;
uint8_t lastP1 = INVALID_TARGET;
uint8_t lastP2 = INVALID_TARGET;
uint32_t cooldownStartTime = 0;

// Variablen für Ripple-Animation
uint32_t animationStartTime = 0;
uint8_t rippleOriginIndex = 0;
uint8_t playerWhoScored = 0; // 1 für P1, 2 für P2

uint32_t gameStartTime = 0;

#include <string.h> // Für strncpy

HighscoreEntry highscores[10];
int8_t lastNewHighscoreIndex = -1; // -1 bedeutet: aktuell kein neuer Highscore zur Benennung offen

// --- Funktions-Prototypen für die State-Handler ---


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
    strncpy(highscores[insertPos].name, HIGHSCORE_PLACEHOLDER_NAME, MAX_NAME_LEN);
    
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
        case IDLE:        handleIdleState(currentTime);       break;
        case PLAYING:     handlePlayingState(currentTime);    break;
        case RIPPLE_ANIM: handleRippleAnimState(currentTime); break;
        case GAME_OVER:   handleGameOverState(currentTime);   break;
        case COOLDOWN:    handleCooldownState(currentTime);   break;
    }
}

/**
 * @brief Behandelt den IDLE-Zustand: Wartet auf Spielstart.
 * Lässt die Tasten für 1-Spieler und 2-Spieler-Modus blinken.
 */
void handleIdleState(uint32_t currentTime) {
    static bool lastBlinkState = false;
    bool currentBlinkState = (currentTime / IDLE_BLINK_INTERVAL_MS) % 2 == 0;
    
    if (currentBlinkState != lastBlinkState) {
        for (int i = 0; i < NUM_FIELDS; i++) leds.set_rgb(OFF, i);
        if (currentBlinkState) {
            leds.set_rgb(COLOR_SINGLE_PLAYER, 0);
            leds.set_rgb(COLOR_MULTIPLAYER_IDLE, 1);
        }
        lastBlinkState = currentBlinkState;
    }

    // Startbedingung prüfen (Taste 0 für Single, Taste 1 für Multi)
    if (isPressed[0] || isPressed[1]) {
        // Spielvariablen zurücksetzen
        scoreP1 = 0;
        scoreP2 = 0;
        lastP1 = INVALID_TARGET; 
        lastP2 = INVALID_TARGET;
        gameStartTime = currentTime;
        
        for (int i = 0; i < NUM_FIELDS; i++) leds.set_rgb(OFF, i);
        
        // Modus festlegen und erste Ziele generieren
        if (isPressed[0]) {
            currentMode = SINGLE_PLAYER;
            set_next_target(1); 
        } else if (isPressed[1]) {
            currentMode = MULTI_PLAYER;
            set_next_target(1);
            set_next_target(2);
        }
        
        gameState = PLAYING;
    }
}

/**
 * @brief Behandelt den PLAYING-Zustand: Hauptspiellogik.
 * Prüft auf Treffer, Zeitablauf oder Spielabbruch.
 */
void handlePlayingState(uint32_t currentTime) {
    if (isGameAbortRequested()) {
        Serial.println("Spiel vorzeitig abgebrochen!");
        gameState = GAME_OVER;
        return;
    }

    if (currentTime - gameStartTime >= GAME_DURATION_MS) {
        gameState = GAME_OVER;
        return;
    }

    // Trefferprüfung für Spieler 1
    if (isPressed[targetP1]) {
        scoreP1++;
        leds.set_rgb(OFF, targetP1);
        
        gameState = RIPPLE_ANIM;
        animationStartTime = currentTime;
        rippleOriginIndex = targetP1;
        playerWhoScored = 1;
        return;
    }
    
    // Trefferprüfung für Spieler 2 (nur im Multiplayer)
    if (currentMode == MULTI_PLAYER && isPressed[targetP2]) {
        scoreP2++;
        leds.set_rgb(OFF, targetP2);

        gameState = RIPPLE_ANIM;
        animationStartTime = currentTime;
        rippleOriginIndex = targetP2;
        playerWhoScored = 2;
        return;
    }
}

/**
 * @brief Behandelt den RIPPLE_ANIM-Zustand: Spielt die Treffer-Animation ab.
 */
void handleRippleAnimState(uint32_t currentTime) {
    uint32_t elapsedTime = currentTime - animationStartTime;

    if (elapsedTime >= RIPPLE_ANIMATION_DURATION_MS) {
        // Animation beendet, zurück zum Spiel
        gameState = PLAYING;
        for (int i = 0; i < NUM_FIELDS; i++) leds.set_rgb(OFF, i);
        set_next_target(playerWhoScored); // Neues Ziel für den Spieler, der getroffen hat

        // Im Multiplayer-Modus das Ziel des anderen Spielers wiederherstellen
        if (currentMode == MULTI_PLAYER) {
            if (playerWhoScored == 1) leds.set_rgb(COLOR_P2, targetP2);
            else leds.set_rgb(COLOR_P1, targetP1);
        }
        return;
    }

    // --- Animationslogik ---
    float progress = (float)elapsedTime / (float)RIPPLE_ANIMATION_DURATION_MS; // 0.0 bis 1.0
    uint8_t originRow = rippleOriginIndex / NUM_COLUMNS;
    uint8_t originCol = rippleOriginIndex % NUM_COLUMNS;
    CRGB rippleBaseColor = (playerWhoScored == 1) ? CRGB(COLOR_P1) : CRGB(COLOR_P2);
    float waveFront = progress * (NUM_COLUMNS / 1.5f); // Welle breitet sich aus

    for (int i = 0; i < NUM_FIELDS; i++) {
        uint8_t col = i % NUM_COLUMNS;
        // Animation auf die Spielerhälfte beschränken
        if (currentMode == MULTI_PLAYER && ((playerWhoScored == 1 && col >= NUM_COLUMNS / 2) || (playerWhoScored == 2 && col < NUM_COLUMNS / 2))) continue;
        
        // Manhattan-Distanz vom Ursprung zum Feld i
        int dist = abs((i / NUM_COLUMNS) - originRow) + abs(col - originCol);
        float delta = fabs((float)dist - waveFront);

        // Helligkeit basierend auf der Nähe zur Wellenfront
        uint8_t brightness = (delta < 1.5f) ? (uint8_t)(255.0f * (1.5f - delta) / 1.5f) : 0;
        
        CRGB finalColor = rippleBaseColor;
        uint8_t fadeOutAlpha = 255 - (uint8_t)(progress * 255);
        finalColor.nscale8(scale8(brightness, fadeOutAlpha));

        leds.set_rgb(finalColor.r << 16 | finalColor.g << 8 | finalColor.b, i);
    }
}

/**
 * @brief Behandelt den GAME_OVER-Zustand: Wertet das Spiel aus und zeigt den Gewinner.
 */
void handleGameOverState(uint32_t currentTime) {
    if (currentMode == SINGLE_PLAYER) {
        Serial.printf("Zeit abgelaufen! Score: %d\n", scoreP1);
        checkAndAddHighscore(scoreP1);
    } else {
        Serial.printf("Zeit abgelaufen! P1: %d | P2: %d\n", scoreP1, scoreP2);
        checkAndAddHighscore(scoreP1);
        checkAndAddHighscore(scoreP2);
    }
    
    displayWinnerScreen();
     
    cooldownStartTime = currentTime;
    gameState = COOLDOWN;
}

/**
 * @brief Behandelt den COOLDOWN-Zustand: Kurze Pause nach dem Spiel.
 */
void handleCooldownState(uint32_t currentTime) {
    if (currentTime - cooldownStartTime >= COOLDOWN_DURATION_MS) {
        for (int i = 0; i < NUM_FIELDS; i++) leds.set_rgb(OFF, i);
        gameState = IDLE; 
    }
}

/**
 * @brief Prüft, ob die Abbruchbedingung (erste 4 Tasten gedrückt) erfüllt ist.
 */
bool isGameAbortRequested() {
    return isPressed[0] && isPressed[1] && isPressed[2] && isPressed[3];
}

/**
 * @brief Steuert die LEDs an, um den Gewinner oder das Single-Player-Ergebnis anzuzeigen.
 */
void displayWinnerScreen() {
    if (currentMode == SINGLE_PLAYER) {
        for (int i = 0; i < NUM_FIELDS; i++) {
            leds.set_rgb(COLOR_SINGLE_PLAYER, i); 
        } 
    } else { // MULTI_PLAYER
        for (int i = 0; i < NUM_FIELDS; i++) {
            uint8_t col = i % NUM_COLUMNS; 
            
            if (scoreP1 > scoreP2) { // P1 gewinnt
                leds.set_rgb((col < NUM_COLUMNS / 2) ? COLOR_P1 : OFF, i);
            } else if (scoreP2 > scoreP1) { // P2 gewinnt
                leds.set_rgb((col >= NUM_COLUMNS / 2) ? COLOR_P2 : OFF, i);
            } else { // Unentschieden
                leds.set_rgb((col < NUM_COLUMNS / 2) ? COLOR_P1 : COLOR_P2, i);
            }
        }
    }
}