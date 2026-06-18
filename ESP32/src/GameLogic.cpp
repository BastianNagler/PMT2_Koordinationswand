#include "GameLogic.h"
#include <cmath>
#include "WebLog.h"

// --- Constants for game-process ---
const uint32_t COOLDOWN_DURATION_MS = 5000;
const uint32_t IDLE_BLINK_INTERVAL_MS = 500;
const uint32_t RIPPLE_ANIMATION_DURATION_MS = 150;
const uint8_t INVALID_TARGET = 99;

GameLogic::GameLogic(volatile bool* inputState, LED_Driver& ledDriver)
    : isPressed(inputState), leds(ledDriver) 
{
}

void GameLogic::init()
{
    highscoreManager.load();
}

uint8_t GameLogic::getRandomGenerator(uint8_t min, uint8_t max)
{
    if (min >= max) return min; 
    
    return esp_random() % (max-min) + min;
}

void GameLogic::set_next_target(uint8_t player)
{
    uint8_t nextField;
    uint8_t* currentTarget = (player == 1) ? &targetP1 : &targetP2;
    uint8_t* lastTarget = (player == 1) ? &lastP1 : &lastP2;
    uint32_t color;

    do {
        if (currentMode == SINGLE_PLAYER) {
            color = settings.colorP1;
            nextField = getRandomGenerator(0, NUM_FIELDS);
        } 
        else {
            color = (player == 1) ? settings.colorP1 : settings.colorP2;
            
            uint8_t randRow = getRandomGenerator(0, NUM_ROWS); 

            uint8_t randCol;
            if (player == 1) {
                randCol = getRandomGenerator(0, NUM_COLUMNS / 2);
            } else {
                randCol = getRandomGenerator(NUM_COLUMNS / 2, NUM_COLUMNS);
            }
            
            nextField = (randRow * NUM_COLUMNS) + randCol;
        }
        
    } while (nextField == *lastTarget);

    *currentTarget = nextField;
    *lastTarget = nextField;
    leds.set_rgb(color, *currentTarget);
}

void GameLogic::run(uint32_t currentTime) {    
    switch (gameState) {
        case IDLE:        handleIdleState(currentTime);       break;
        case START_ANIM:  handleStartAnimState(currentTime);  break;
        case PLAYING:     handlePlayingState(currentTime);    break;
        case RIPPLE_ANIM: handleRippleAnimState(currentTime); break;
        case GAME_OVER:   handleGameOverState(currentTime);   break;
        case COOLDOWN:    handleCooldownState(currentTime);   break;
    }
}

void GameLogic::handleIdleState(uint32_t currentTime) {
    static bool lastBlinkState = false;
    bool currentBlinkState = (currentTime / IDLE_BLINK_INTERVAL_MS) % 2 == 0;
    
    if (currentBlinkState != lastBlinkState) {
        for (int i = 0; i < NUM_FIELDS; i++) leds.set_rgb(OFF, i);
        if (currentBlinkState) {
            leds.set_rgb(settings.colorP1, 0);
            leds.set_rgb(settings.colorP2, 7);
            leds.set_rgb(settings.colorP2, 15);
        }
        lastBlinkState = currentBlinkState;
    }

    // check starting conditions (button 0 for singleplayer, button 7 or 15 for multiplayer)
    bool pressed0 = isPressed[0];
    bool pressed7 = isPressed[7];
    bool pressed15 = isPressed[15];

    if (pressed0 || pressed7 || pressed15) {
        // reset game variables
        scoreP1 = 0;
        scoreP2 = 0;
        lastP1 = INVALID_TARGET; 
        lastP2 = INVALID_TARGET;
        
        for (int i = 0; i < NUM_FIELDS; i++) leds.set_rgb(OFF, i);
        
        // set mode
        if (pressed0) {
            currentMode = SINGLE_PLAYER;
            lastP1 = 0; // Prevent first target on start button
            WebLog.println("[GAME] Start Singleplayer (triggered by button 0)");
        } else if (pressed7 || pressed15) {
            currentMode = MULTI_PLAYER;
            WebLog.printf("[GAME] Start Multiplayer (triggered by button %d)\n", pressed7 ? 7 : 15);
            if(pressed7)
            {
                lastP2 = 7;
            }
            else
            {
                lastP2 = 15;
            }
        }
        
        startAnimStartTime = currentTime;
        gameState = START_ANIM;
    }
}

void GameLogic::handleStartAnimState(uint32_t currentTime) {
    uint32_t elapsed = currentTime - startAnimStartTime;

    if (elapsed < 1000) {
        // First row Red
        for (int i = 0; i < NUM_COLUMNS; i++) leds.set_rgb(RED, i);
    } else if (elapsed < 2000) {
        // First row Red
        for (int i = 0; i < NUM_COLUMNS; i++) leds.set_rgb(RED, i);
        // Middle rows Orange
        for (int r = 1; r < NUM_ROWS - 1; r++) {
            for (int c = 0; c < NUM_COLUMNS; c++) leds.set_rgb(ORANGE, r * NUM_COLUMNS + c);
        }
    } else if (elapsed < 3000) {
        for (int i = 0; i < NUM_COLUMNS; i++) leds.set_rgb(RED, i);
        for (int r = 1; r < NUM_ROWS - 1; r++) {
            for (int c = 0; c < NUM_COLUMNS; c++) leds.set_rgb(ORANGE, r * NUM_COLUMNS + c);
        }
        // Last row Green
        for (int c = 0; c < NUM_COLUMNS; c++) leds.set_rgb(GREEN, (NUM_ROWS - 1) * NUM_COLUMNS + c);
    } else {
        // Animation done
        for (int i = 0; i < NUM_FIELDS; i++) leds.set_rgb(OFF, i);
        
        gameStartTime = currentTime;
        
        if (currentMode == SINGLE_PLAYER) {
            set_next_target(1); 
        } else {
            set_next_target(1);
            set_next_target(2);
        }
        
        gameState = PLAYING;
    }
}

void GameLogic::handlePlayingState(uint32_t currentTime) {
    if (isGameAbortRequested()) {
        WebLog.println("Spiel vorzeitig abgebrochen!");
        gameState = GAME_OVER;
        return;
    }

    if (currentTime - gameStartTime >= settings.gameDurationMs) {
        gameState = GAME_OVER;
        return;
    }

    // player 1 check hit
    if (targetP1 < NUM_FIELDS && isPressed[targetP1]) {
        scoreP1 += 1;
        leds.set_rgb(OFF, targetP1);
        
        gameState = RIPPLE_ANIM;
        animationStartTime = currentTime;
        rippleOriginIndex = targetP1;
        playerWhoScored = 1;
        return;
    }
    
    // player 2 check hit (multiplayer only)
    if (currentMode == MULTI_PLAYER && targetP2 < NUM_FIELDS && isPressed[targetP2]) {
        scoreP2 += 1;
        leds.set_rgb(OFF, targetP2);

        gameState = RIPPLE_ANIM;
        animationStartTime = currentTime;
        rippleOriginIndex = targetP2;
        playerWhoScored = 2;
        return;
    }
}

void GameLogic::handleRippleAnimState(uint32_t currentTime) {
    uint32_t elapsedTime = currentTime - animationStartTime;

    if (elapsedTime >= RIPPLE_ANIMATION_DURATION_MS) {
        // animation over, return to playing mode
        gameState = PLAYING;
        for (int i = 0; i < NUM_FIELDS; i++) leds.set_rgb(OFF, i);
        set_next_target(playerWhoScored); // set new target

        // relight other players target for multiplayer
        if (currentMode == MULTI_PLAYER) {
            if (playerWhoScored == 1) leds.set_rgb(settings.colorP2, targetP2);
            else leds.set_rgb(settings.colorP1, targetP1);
        }
        return;
    }

    // --- animation logic ---
    float progress = (float)elapsedTime / (float)RIPPLE_ANIMATION_DURATION_MS; // 0.0 thru 1.0
    uint8_t originRow = rippleOriginIndex / NUM_COLUMNS;
    uint8_t originCol = rippleOriginIndex % NUM_COLUMNS;
    CRGB rippleBaseColor = (playerWhoScored == 1) ? CRGB(settings.colorP1Ripple) : CRGB(settings.colorP2Ripple);
    float waveFront = progress * (NUM_COLUMNS / 1.5f); // waveform expanding

    for (int i = 0; i < NUM_FIELDS; i++) {
        uint8_t col = i % NUM_COLUMNS;
        // limit animation to one half
        if (currentMode == MULTI_PLAYER && ((playerWhoScored == 1 && col >= NUM_COLUMNS / 2) || (playerWhoScored == 2 && col < NUM_COLUMNS / 2))) continue;
        
        // Manhattan-distance to field 1
        int dist = abs((i / NUM_COLUMNS) - originRow) + abs(col - originCol);
        float delta = fabs((float)dist - waveFront);

        // brightness based on proximity to wavefront
        uint8_t brightness = (delta < 1.5f) ? (uint8_t)(255.0f * (1.5f - delta) / 1.5f) : 0;
        
        CRGB finalColor = rippleBaseColor;
        uint8_t fadeOutAlpha = 255 - (uint8_t)(progress * 255);
        finalColor.nscale8(scale8(brightness, fadeOutAlpha));

        leds.set_rgb(finalColor.r << 16 | finalColor.g << 8 | finalColor.b, i);
    }
}

void GameLogic::handleGameOverState(uint32_t currentTime) {
    if (currentMode == SINGLE_PLAYER) {
        WebLog.printf("Zeit abgelaufen! Score: %d\n", scoreP1);
        highscoreManager.checkAndAdd(scoreP1, settings.gameDurationMs == 60000);
    } else {
        WebLog.printf("Zeit abgelaufen! P1: %d | P2: %d\n", scoreP1, scoreP2);
    }
    
    displayWinnerScreen();
     
    cooldownStartTime = currentTime;
    gameState = COOLDOWN;
}

void GameLogic::handleCooldownState(uint32_t currentTime) {
    if (currentTime - cooldownStartTime >= COOLDOWN_DURATION_MS) {
        for (int i = 0; i < NUM_FIELDS; i++) leds.set_rgb(OFF, i);
        gameState = IDLE; 
    }
}

bool GameLogic::isGameAbortRequested() {
#if NUM_FIELDS >= 4
    return isPressed[0] && isPressed[1] && isPressed[2] && isPressed[3];
#else
    return false;
#endif
}

void GameLogic::displayWinnerScreen() {
    if (currentMode == SINGLE_PLAYER) {
        for (int i = 0; i < NUM_FIELDS; i++) {
            leds.set_rgb(settings.colorP1, i); 
        } 
    } else { // multiplayer
        for (int i = 0; i < NUM_FIELDS; i++) {
            uint8_t col = i % NUM_COLUMNS; 
            
            if (scoreP1 > scoreP2) { // P1 wins
                leds.set_rgb((col < NUM_COLUMNS / 2) ? settings.colorP1 : OFF, i);
            } else if (scoreP2 > scoreP1) { // P2 wins
                leds.set_rgb((col >= NUM_COLUMNS / 2) ? settings.colorP2 : OFF, i);
            } else { // tie
                leds.set_rgb((col < NUM_COLUMNS / 2) ? settings.colorP1 : settings.colorP2, i);
            }
        }
    }
}

void GameLogic::updateHighscoreName(const int index, const char* newName, bool isDefaultTime) {
    highscoreManager.updateHighscoreName(index, newName, isDefaultTime);
}

void GameLogic::applyNewDuration(uint32_t newDuration) {
    if (settings.gameDurationMs != newDuration) {
        settings.gameDurationMs = newDuration;
        if (newDuration != 60000) {
            highscoreManager.clearDynamicHighscores();
        }
    }
}