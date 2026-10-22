#pragma once

#include <Arduino.h>
#include <cstdint>

/**
 * Initializes WiFi SoftAP, LittleFS filesystem, AsyncWebServer, and WebSocket endpoints.
 */
void initWebInterface();

/**
 * Cleans up disconnected WebSocket clients from memory and flushes queued logs.
 */
void cleanupWebInterface();

/**
 * Broadcasts match start event with selected mode and countdown duration to all clients.
 *
 * @param[in] mode String identifier ("single" or "multi").
 */
void notifyGameStart(const char* const mode);

/**
 * Broadcasts updated player scores to all connected dashboard clients.
 *
 * @param[in] scoreP1 Current score count for Player 1.
 * @param[in] scoreP2 Current score count for Player 2.
 */
void updateScore(const uint8_t scoreP1, const uint8_t scoreP2);

/**
 * Broadcasts match completion event including final scores and refreshed highscore leaderboard.
 */
void notifyGameOver();
