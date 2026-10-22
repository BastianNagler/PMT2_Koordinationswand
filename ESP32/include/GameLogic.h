#pragma once

#include <Arduino.h>
#include <array>
#include <cstdint>
#include <mutex>
#include "HighscoreManager.h"
#include "config.h"
#include "leds.h"

// ============================================================================
// Standard Color Definitions (24-bit 0x00RRGGBB)
// ============================================================================

inline constexpr uint32_t COLOR_RED     = 0x00FF0000;
inline constexpr uint32_t COLOR_GREEN   = 0x0000FF00;
inline constexpr uint32_t COLOR_BLUE    = 0x000000FF;
inline constexpr uint32_t COLOR_YELLOW  = 0x00FFFF00;
inline constexpr uint32_t COLOR_CYAN    = 0x0000FFFF;
inline constexpr uint32_t COLOR_MAGENTA = 0x00FF00FF;
inline constexpr uint32_t COLOR_ORANGE  = 0x00FFA500;
inline constexpr uint32_t COLOR_PURPLE  = 0x00800080;
inline constexpr uint32_t COLOR_WHITE   = 0x00FFFFFF;
inline constexpr uint32_t COLOR_GRAY    = 0x00AAAAAA;
inline constexpr uint32_t COLOR_OFF     = 0x00000000;

/**
 * Operational states of the cWall reaction game state machine.
 */
enum GameState {
    IDLE,
    START_ANIM,
    PLAYING,
    RIPPLE_ANIM,
    GAME_OVER,
    COOLDOWN
};

/**
 * Supported game play modes.
 */
enum GameMode {
    SINGLE_PLAYER,
    MULTI_PLAYER
};

/**
 * Configurable game settings adjustable via the web dashboard.
 */
struct GameSettings {
    uint32_t gameDurationMs = 60000;
    uint32_t colorStartAnim = COLOR_GREEN;
    uint32_t colorP1 = COLOR_RED;
    uint32_t colorP2 = COLOR_BLUE;
    uint32_t colorP1Ripple = 0x00808080;
    uint32_t colorP2Ripple = 0x00808080;
};

/**
 * Core game engine orchestrating input sensing, target generation, and animation.
 */
class GameLogic {
private:
    volatile bool* const isPressed;
    LED_Driver& leds;
    HighscoreManager highscoreManager;

    mutable std::mutex stateMutex;

    GameState gameState = IDLE;
    GameMode currentMode = SINGLE_PLAYER;

    uint8_t scoreP1 = 0;
    uint8_t scoreP2 = 0;
    uint8_t targetP1 = NUM_FIELDS + 1;
    uint8_t targetP2 = NUM_FIELDS + 1;
    uint8_t lastP1 = NUM_FIELDS + 1;
    uint8_t lastP2 = NUM_FIELDS + 1;

    uint32_t cooldownStartTime = 0;
    uint32_t startAnimStartTime = 0;
    uint32_t animationStartTime = 0;
    uint8_t rippleOriginIndex = 0;
    uint8_t playerWhoScored = 0;
    uint32_t gameStartTime = 0;

    uint8_t getRandomGenerator(const uint8_t min, const uint8_t max);
    void set_next_target(const uint8_t player);
    void handleIdleState(const uint32_t currentTime);
    void handleStartAnimState(const uint32_t currentTime);
    void handlePlayingState(const uint32_t currentTime);
    void handleRippleAnimState(const uint32_t currentTime);
    void handleGameOverState(const uint32_t currentTime);
    void handleCooldownState(const uint32_t currentTime);
    bool isGameAbortRequested() const;
    void displayWinnerScreen();

public:
    GameSettings settings;

    /**
     * Constructs the GameLogic engine with references to shared sensor states and LED driver.
     *
     * @param[in] inputState Pointer to volatile sensor pressed state array.
     * @param[in] ledDriver Reference to the hardware LED driver.
     */
    GameLogic(volatile bool* const inputState, LED_Driver& ledDriver);

    ~GameLogic() = default;
    GameLogic(const GameLogic&) = delete;
    GameLogic& operator=(const GameLogic&) = delete;

    /**
     * Initializes highscore records and prepares the game state.
     */
    void init();

    /**
     * Executes one cycle of the game state machine.
     *
     * @param[in] currentTime Current system timestamp in milliseconds from `millis()`.
     */
    void run(const uint32_t currentTime);

    /**
     * Queries the current operational game state in a thread-safe manner.
     *
     * @return Active `GameState`.
     */
    GameState getGameState() const;

    /**
     * Queries the active game mode.
     *
     * @return `SINGLE_PLAYER` or `MULTI_PLAYER`.
     */
    GameMode getGameMode() const;

    /**
     * Retrieves current score for Player 1.
     *
     * @return Numerical score count.
     */
    uint8_t getScoreP1() const;

    /**
     * Retrieves current score for Player 2 (multiplayer).
     *
     * @return Numerical score count.
     */
    uint8_t getScoreP2() const;

    /**
     * Retrieves the timestamp when the current match started.
     *
     * @return Start timestamp in milliseconds.
     */
    uint32_t getGameStartTime() const;

    /**
     * Obtains a thread-safe copy of the requested highscore table.
     *
     * @param[in] isDefaultTime `true` for standard 60s table, `false` for dynamic table.
     *
     * @return Statically sized array containing 10 highscore records.
     */
    std::array<HighscoreEntry, NUM_HIGHSCORES> getHighscoresCopy(const bool isDefaultTime) const;

    /**
     * Updates player name for a highscore entry.
     *
     * @param[in] index Table ranking slot index (0..9).
     * @param[in] newName Null-terminated player name string.
     * @param[in] isDefaultTime Target table identifier.
     */
    void updateHighscoreName(const int index, const char* const newName, const bool isDefaultTime);

    /**
     * Validates and applies a new game duration setting.
     *
     * @param[in] newDuration Target match duration in milliseconds (5000 to 600000).
     */
    void applyNewDuration(const uint32_t newDuration);

    /**
     * Resets persistent 60-second highscores back to factory defaults.
     */
    void reset60sHighscore();

    /**
     * Atomically sets game visual colors.
     *
     * @param[in] startAnim Color for countdown animation.
     * @param[in] p1 Color for Player 1.
     * @param[in] p2 Color for Player 2.
     * @param[in] p1Ripple Color for Player 1 ripple wave.
     * @param[in] p2Ripple Color for Player 2 ripple wave.
     */
    void setColors(const uint32_t startAnim, const uint32_t p1, const uint32_t p2,
                   const uint32_t p1Ripple, const uint32_t p2Ripple);
};
