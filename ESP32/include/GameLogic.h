#pragma once

#include <Arduino.h>
#include "config.h"
#include "leds.h"
#include "HighscoreManager.h"

#define RED     0x00FF0000
#define GREEN   0x0000FF00
#define BLUE    0x000000FF
#define YELLOW  0x00FFFF00
#define CYAN    0x0000FFFF
#define MAGENTA 0x00FF00FF
#define ORANGE  0x00FFA500
#define PURPLE  0x00800080
#define WHITE   0x00FFFFFF
#define GRAY    0x00AAAAAA
#define OFF     0x00000000

enum GameState { IDLE, PLAYING, RIPPLE_ANIM, GAME_OVER, COOLDOWN };
enum GameMode { SINGLE_PLAYER, MULTI_PLAYER };

struct GameSettings {
    uint32_t gameDurationMs = 60000;
    uint32_t colorSinglePlayer = GREEN;
    uint32_t colorMultiplayerIdle = BLUE;
    uint32_t colorP1 = MAGENTA;
    uint32_t colorP2 = ORANGE;
    uint32_t colorP1Ripple = WHITE;
    uint32_t colorP2Ripple = WHITE;
};

class GameLogic {
public:
    GameLogic(volatile bool* inputState, LED_Driver& ledDriver);
    
    void init();
    void run(uint32_t currentTime);
    
    GameSettings settings;
    
    GameState getGameState() const { return gameState; }
    GameMode getGameMode() const { return currentMode; }
    uint8_t getScoreP1() const { return scoreP1; }
    uint8_t getScoreP2() const { return scoreP2; }
    const HighscoreEntry* getHighscores(bool isDefaultTime) const { return highscoreManager.getHighscores(isDefaultTime); }
    
    void updateHighscoreName(const int index, const char* newName, bool isDefaultTime);
    void applyNewDuration(uint32_t newDuration);
    void reset60sHighscore() { highscoreManager.reset60sHighscore(); }

private:
    volatile bool* isPressed;
    LED_Driver& leds;
    HighscoreManager highscoreManager;

    volatile GameState gameState = IDLE;
    volatile GameMode currentMode = SINGLE_PLAYER;

    volatile uint8_t scoreP1 = 0;
    volatile uint8_t scoreP2 = 0;
    uint8_t targetP1 = 99; // INVALID_TARGET 
    uint8_t targetP2 = 99; // INVALID_TARGET
    uint8_t lastP1 = 99;   // INVALID_TARGET
    uint8_t lastP2 = 99;   // INVALID_TARGET
    uint32_t cooldownStartTime = 0;

    uint32_t animationStartTime = 0;
    uint8_t rippleOriginIndex = 0;
    uint8_t playerWhoScored = 0; 
    uint32_t gameStartTime = 0;

    uint8_t getRandomGenerator(uint8_t min, uint8_t max);
    void set_next_target(uint8_t player);
    void handleIdleState(uint32_t currentTime);
    void handlePlayingState(uint32_t currentTime);
    void handleRippleAnimState(uint32_t currentTime);
    void handleGameOverState(uint32_t currentTime);
    void handleCooldownState(uint32_t currentTime);
    bool isGameAbortRequested();
    void displayWinnerScreen();
};