#pragma once

#include <Arduino.h>
#include <LittleFS.h>
#include <array>
#include <cstdint>
#include <cstddef>
#include <mutex>

/**
 * Maximum character length for a player name including null terminator.
 */
inline constexpr size_t MAX_NAME_LEN = 16;

/**
 * Total entries held in a highscore table.
 */
inline constexpr size_t NUM_HIGHSCORES = 10;

/**
 * Single highscore record storing the numerical score and player name.
 */
struct HighscoreEntry {
    uint8_t score = 0;
    char name[MAX_NAME_LEN] = "---";
};

/**
 * Manages persistent (LittleFS) and dynamic game highscores with thread safety.
 */
class HighscoreManager {
private:
    std::array<HighscoreEntry, NUM_HIGHSCORES> highscores60{};
    std::array<HighscoreEntry, NUM_HIGHSCORES> highscoresDynamic{};
    mutable std::recursive_mutex mtx;

    /**
     * Serializes standard 60-second highscore table to LittleFS binary file.
     */
    void save60();

public:
    HighscoreManager() = default;
    ~HighscoreManager() = default;
    HighscoreManager(const HighscoreManager&) = delete;
    HighscoreManager& operator=(const HighscoreManager&) = delete;

    /**
     * Loads highscores from LittleFS storage or re-initializes table if absent or corrupt.
     */
    void load();

    /**
     * Checks if score qualifies for Top 10 and inserts it if eligible.
     *
     * @param[in] newScore Numerical score achieved in game.
     * @param[in] isDefaultTime `true` if played under standard 60s rule, `false` for custom durations.
     */
    void checkAndAdd(const uint8_t newScore, const bool isDefaultTime);

    /**
     * Updates player name for a specific ranking entry in the highscore table.
     *
     * @param[in] index Zero-based ranking index (0..9).
     * @param[in] newName Null-terminated player name string.
     * @param[in] isDefaultTime `true` for 60s table, `false` for dynamic table.
     */
    void updateHighscoreName(const int index, const char* const newName, const bool isDefaultTime);

    /**
     * Returns a thread-safe copy of the requested highscore table.
     *
     * @param[in] isDefaultTime `true` for 60s table, `false` for dynamic table.
     *
     * @return Statically sized array containing 10 highscore entries.
     */
    std::array<HighscoreEntry, NUM_HIGHSCORES> getHighscoresCopy(const bool isDefaultTime) const;

    /**
     * Resets the non-standard duration in-memory highscore table to default placeholder entries.
     */
    void clearDynamicHighscores();

    /**
     * Clears persistent 60s highscore table and rewrites the default entries to LittleFS.
     */
    void reset60sHighscore();
};
