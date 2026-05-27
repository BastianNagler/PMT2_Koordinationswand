#pragma once

#include <Arduino.h>
#include "config.h"
#include <Preferences.h>
#include "leds.h"

// --- Farb-Definitionen (am besten hier zentral ablegen) ---
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

// --- Enums ---
enum GameState { IDLE, PLAYING, RIPPLE_ANIM, GAME_OVER, COOLDOWN };
enum GameMode { SINGLE_PLAYER, MULTI_PLAYER };

// --- Externe Variablen (werden im Hauptprogramm definiert) ---
// Damit die GameLogic auf die LEDs und die Tasten zugreifen kann
extern volatile bool isPressed[NUM_FIELDS];
extern LED_Driver leds;

extern GameState gameState;
extern GameMode currentMode;

// Maximale Länge für den Spielernamen (inkl. Null-Terminator)
#define MAX_NAME_LEN 16 

struct HighscoreEntry {
    uint8_t score;
    char name[MAX_NAME_LEN];
};

extern HighscoreEntry highscores[10];
extern int8_t lastNewHighscoreIndex; 
void updateLastHighscoreName(const char* newName);
void loadHighscores();
void saveHighscores();
void checkAndAddHighscore(uint8_t newScore);
// --- Funktions-Prototypen ---
uint8_t getRandomGenerator(uint8_t min, uint8_t max);
void set_next_target(uint8_t player);
void runGameLogic(uint32_t currentTime);
void handleIdleState(uint32_t currentTime);
void handlePlayingState(uint32_t currentTime);
void handleRippleAnimState(uint32_t currentTime);
void handleGameOverState(uint32_t currentTime);
void handleCooldownState(uint32_t currentTime);
bool isGameAbortRequested();
void displayWinnerScreen();