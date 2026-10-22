#include "GameLogic.h"
#include <cmath>
#include "WebLog.h"

static constexpr uint32_t COOLDOWN_DURATION_MS = 5000;
static constexpr uint32_t IDLE_BLINK_INTERVAL_MS = 500;
static constexpr uint32_t RIPPLE_ANIMATION_DURATION_MS = 150;
static constexpr uint8_t INVALID_TARGET = 99;

GameLogic::GameLogic(volatile bool* const inputState, LED_Driver& ledDriver)
    : isPressed(inputState), leds(ledDriver) {}

void GameLogic::init() {
    std::lock_guard<std::mutex> lock(stateMutex);
    highscoreManager.load();
}

uint8_t GameLogic::getRandomGenerator(const uint8_t min, const uint8_t max) {
    if (min >= max) {
        return min;
    }
    return static_cast<uint8_t>(esp_random() % (max - min) + min);
}

void GameLogic::set_next_target(const uint8_t player) {
    uint8_t nextField = 0;
    uint8_t* const currentTarget = (player == 1) ? &targetP1 : &targetP2;
    uint8_t* const lastTarget = (player == 1) ? &lastP1 : &lastP2;
    uint32_t targetColor = 0;

    do {
        if (currentMode == SINGLE_PLAYER) {
            targetColor = settings.colorP1;
            nextField = getRandomGenerator(0, NUM_FIELDS);
        } else {
            targetColor = (player == 1) ? settings.colorP1 : settings.colorP2;
            const uint8_t randRow = getRandomGenerator(0, NUM_ROWS);
            const uint8_t randCol = (player == 1)
                                        ? getRandomGenerator(0, NUM_COLUMNS / 2)
                                        : getRandomGenerator(NUM_COLUMNS / 2, NUM_COLUMNS);
            nextField = static_cast<uint8_t>((randRow * NUM_COLUMNS) + randCol);
        }
    } while (nextField == *lastTarget);

    *currentTarget = nextField;
    *lastTarget = nextField;
    leds.set_rgb(targetColor, *currentTarget);
}

void GameLogic::run(const uint32_t currentTime) {
    std::lock_guard<std::mutex> lock(stateMutex);

    switch (gameState) {
        case IDLE:        handleIdleState(currentTime);       break;
        case START_ANIM:  handleStartAnimState(currentTime);  break;
        case PLAYING:     handlePlayingState(currentTime);    break;
        case RIPPLE_ANIM: handleRippleAnimState(currentTime); break;
        case GAME_OVER:   handleGameOverState(currentTime);   break;
        case COOLDOWN:    handleCooldownState(currentTime);   break;
    }
}

void GameLogic::handleIdleState(const uint32_t currentTime) {
    static bool lastBlinkState = false;
    const bool currentBlinkState = ((currentTime / IDLE_BLINK_INTERVAL_MS) % 2) == 0;

    if (currentBlinkState != lastBlinkState) {
        for (uint8_t i = 0; i < NUM_FIELDS; ++i) {
            leds.set_rgb(COLOR_OFF, i);
        }
        if (currentBlinkState) {
            leds.set_rgb(settings.colorP1, 0);
            leds.set_rgb(settings.colorP2, 7);
            leds.set_rgb(settings.colorP2, 15);
        }
        lastBlinkState = currentBlinkState;
    }

    const bool pressed0 = isPressed[0];
    const bool pressed7 = (NUM_FIELDS > 7) ? isPressed[7] : false;
    const bool pressed15 = (NUM_FIELDS > 15) ? isPressed[15] : false;

    if (pressed0 || pressed7 || pressed15) {
        scoreP1 = 0;
        scoreP2 = 0;
        lastP1 = INVALID_TARGET;
        lastP2 = INVALID_TARGET;

        for (uint8_t i = 0; i < NUM_FIELDS; ++i) {
            leds.set_rgb(COLOR_OFF, i);
        }

        if (pressed0) {
            currentMode = SINGLE_PLAYER;
            lastP1 = 0;
            WebLog.println("[GAME] Start Singleplayer (button 0)");
        } else {
            currentMode = MULTI_PLAYER;
            WebLog.printf("[GAME] Start Multiplayer (button %d)\n", pressed7 ? 7 : 15);
            lastP2 = pressed7 ? 7 : 15;
        }

        startAnimStartTime = currentTime;
        gameState = START_ANIM;
    }
}

void GameLogic::handleStartAnimState(const uint32_t currentTime) {
    const uint32_t elapsed = currentTime - startAnimStartTime;
    constexpr uint32_t intervalMs = 500;
    const uint32_t maxElapsed = NUM_ROWS * intervalMs;

    if (elapsed < maxElapsed) {
        uint32_t rowsToLight = (elapsed / intervalMs) + 1;
        if (rowsToLight > NUM_ROWS) {
            rowsToLight = NUM_ROWS;
        }

        for (uint8_t r = 0; r < rowsToLight; ++r) {
            for (uint8_t c = 0; c < NUM_COLUMNS; ++c) {
                leds.set_rgb(settings.colorStartAnim, static_cast<uint8_t>(r * NUM_COLUMNS + c));
            }
        }
    } else {
        for (uint8_t i = 0; i < NUM_FIELDS; ++i) {
            leds.set_rgb(COLOR_OFF, i);
        }

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

void GameLogic::handlePlayingState(const uint32_t currentTime) {
    if (isGameAbortRequested()) {
        WebLog.println("[GAME] Game aborted by user");
        gameState = GAME_OVER;
        return;
    }

    if (currentTime - gameStartTime >= settings.gameDurationMs) {
        gameState = GAME_OVER;
        return;
    }

    if (targetP1 < NUM_FIELDS && isPressed[targetP1]) {
        scoreP1 += 1;
        leds.set_rgb(COLOR_OFF, targetP1);

        gameState = RIPPLE_ANIM;
        animationStartTime = currentTime;
        rippleOriginIndex = targetP1;
        playerWhoScored = 1;
        return;
    }

    if (currentMode == MULTI_PLAYER && targetP2 < NUM_FIELDS && isPressed[targetP2]) {
        scoreP2 += 1;
        leds.set_rgb(COLOR_OFF, targetP2);

        gameState = RIPPLE_ANIM;
        animationStartTime = currentTime;
        rippleOriginIndex = targetP2;
        playerWhoScored = 2;
        return;
    }
}

void GameLogic::handleRippleAnimState(const uint32_t currentTime) {
    const uint32_t elapsedTime = currentTime - animationStartTime;

    if (elapsedTime >= RIPPLE_ANIMATION_DURATION_MS) {
        gameState = PLAYING;
        for (uint8_t i = 0; i < NUM_FIELDS; ++i) {
            leds.set_rgb(COLOR_OFF, i);
        }
        set_next_target(playerWhoScored);

        if (currentMode == MULTI_PLAYER) {
            if (playerWhoScored == 1) {
                leds.set_rgb(settings.colorP2, targetP2);
            } else {
                leds.set_rgb(settings.colorP1, targetP1);
            }
        }
        return;
    }

    const float progress = static_cast<float>(elapsedTime) / static_cast<float>(RIPPLE_ANIMATION_DURATION_MS);
    const uint8_t originRow = static_cast<uint8_t>(rippleOriginIndex / NUM_COLUMNS);
    const uint8_t originCol = static_cast<uint8_t>(rippleOriginIndex % NUM_COLUMNS);
    const CRGB rippleBaseColor = (playerWhoScored == 1) ? CRGB(settings.colorP1Ripple) : CRGB(settings.colorP2Ripple);
    const float waveFront = progress * (NUM_COLUMNS / 1.5f);

    for (uint8_t i = 0; i < NUM_FIELDS; ++i) {
        const uint8_t col = static_cast<uint8_t>(i % NUM_COLUMNS);
        if (currentMode == MULTI_PLAYER &&
            ((playerWhoScored == 1 && col >= NUM_COLUMNS / 2) ||
             (playerWhoScored == 2 && col < NUM_COLUMNS / 2))) {
            continue;
        }

        const int dist = abs(static_cast<int>(i / NUM_COLUMNS) - static_cast<int>(originRow)) +
                         abs(static_cast<int>(col) - static_cast<int>(originCol));
        const float delta = fabs(static_cast<float>(dist) - waveFront);

        const uint8_t brightness = (delta < 1.5f)
                                       ? static_cast<uint8_t>(255.0f * (1.5f - delta) / 1.5f)
                                       : 0;

        CRGB finalColor = rippleBaseColor;
        const uint8_t fadeOutAlpha = static_cast<uint8_t>(255 - static_cast<uint8_t>(progress * 255));
        finalColor.nscale8(scale8(brightness, fadeOutAlpha));

        leds.set_rgb((static_cast<uint32_t>(finalColor.r) << 16) |
                     (static_cast<uint32_t>(finalColor.g) << 8) |
                      static_cast<uint32_t>(finalColor.b), i);
    }
}

void GameLogic::handleGameOverState(const uint32_t currentTime) {
    if (currentMode == SINGLE_PLAYER) {
        WebLog.printf("[GAME] Time up! Score: %d\n", scoreP1);
        highscoreManager.checkAndAdd(scoreP1, settings.gameDurationMs == 60000);
    } else {
        WebLog.printf("[GAME] Time up! P1: %d | P2: %d\n", scoreP1, scoreP2);
    }

    displayWinnerScreen();
    cooldownStartTime = currentTime;
    gameState = COOLDOWN;
}

void GameLogic::handleCooldownState(const uint32_t currentTime) {
    if (currentTime - cooldownStartTime >= COOLDOWN_DURATION_MS) {
        for (uint8_t i = 0; i < NUM_FIELDS; ++i) {
            leds.set_rgb(COLOR_OFF, i);
        }
        gameState = IDLE;
    }
}

bool GameLogic::isGameAbortRequested() const {
#if NUM_FIELDS >= 4
    return isPressed[0] && isPressed[1] && isPressed[2] && isPressed[3];
#else
    return false;
#endif
}

void GameLogic::displayWinnerScreen() {
    if (currentMode == SINGLE_PLAYER) {
        for (uint8_t i = 0; i < NUM_FIELDS; ++i) {
            leds.set_rgb(settings.colorP1, i);
        }
    } else {
        for (uint8_t i = 0; i < NUM_FIELDS; ++i) {
            const uint8_t col = static_cast<uint8_t>(i % NUM_COLUMNS);
            if (scoreP1 > scoreP2) {
                leds.set_rgb((col < NUM_COLUMNS / 2) ? settings.colorP1 : COLOR_OFF, i);
            } else if (scoreP2 > scoreP1) {
                leds.set_rgb((col >= NUM_COLUMNS / 2) ? settings.colorP2 : COLOR_OFF, i);
            } else {
                leds.set_rgb((col < NUM_COLUMNS / 2) ? settings.colorP1 : settings.colorP2, i);
            }
        }
    }
}

GameState GameLogic::getGameState() const {
    std::lock_guard<std::mutex> lock(stateMutex);
    return gameState;
}

GameMode GameLogic::getGameMode() const {
    std::lock_guard<std::mutex> lock(stateMutex);
    return currentMode;
}

uint8_t GameLogic::getScoreP1() const {
    std::lock_guard<std::mutex> lock(stateMutex);
    return scoreP1;
}

uint8_t GameLogic::getScoreP2() const {
    std::lock_guard<std::mutex> lock(stateMutex);
    return scoreP2;
}

uint32_t GameLogic::getGameStartTime() const {
    std::lock_guard<std::mutex> lock(stateMutex);
    return gameStartTime;
}

std::array<HighscoreEntry, NUM_HIGHSCORES> GameLogic::getHighscoresCopy(const bool isDefaultTime) const {
    std::lock_guard<std::mutex> lock(stateMutex);
    return highscoreManager.getHighscoresCopy(isDefaultTime);
}

void GameLogic::updateHighscoreName(const int index, const char* const newName, const bool isDefaultTime) {
    std::lock_guard<std::mutex> lock(stateMutex);
    highscoreManager.updateHighscoreName(index, newName, isDefaultTime);
}

void GameLogic::applyNewDuration(const uint32_t newDuration) {
    std::lock_guard<std::mutex> lock(stateMutex);
    if (newDuration < 5000 || newDuration > 600000) {
        WebLog.printf("[ERROR] applyNewDuration: invalid duration %u ms rejected\n", newDuration);
        return;
    }
    if (settings.gameDurationMs != newDuration) {
        settings.gameDurationMs = newDuration;
        if (newDuration != 60000) {
            highscoreManager.clearDynamicHighscores();
        }
    }
}

void GameLogic::reset60sHighscore() {
    std::lock_guard<std::mutex> lock(stateMutex);
    highscoreManager.reset60sHighscore();
}

void GameLogic::setColors(const uint32_t startAnim, const uint32_t p1, const uint32_t p2,
                         const uint32_t p1Ripple, const uint32_t p2Ripple) {
    std::lock_guard<std::mutex> lock(stateMutex);
    settings.colorStartAnim = startAnim;
    settings.colorP1 = p1;
    settings.colorP2 = p2;
    settings.colorP1Ripple = p1Ripple;
    settings.colorP2Ripple = p2Ripple;
}
