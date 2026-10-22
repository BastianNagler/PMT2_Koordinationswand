#include "HighscoreManager.h"
#include <cstring>
#include "WebLog.h"

static constexpr const char* HIGHSCORE_PLACEHOLDER_NAME = "Insert Name";
static constexpr const char* HIGHSCORE_FILE_PATH = "/highscores.dat";

void HighscoreManager::load() {
    std::lock_guard<std::recursive_mutex> lock(mtx);

    if (LittleFS.exists(HIGHSCORE_FILE_PATH)) {
        File file = LittleFS.open(HIGHSCORE_FILE_PATH, "r");
        if (file) {
            const size_t bytesRead = file.read(reinterpret_cast<uint8_t*>(highscores60), sizeof(highscores60));
            file.close();
            if (bytesRead == sizeof(highscores60)) {
                WebLog.println("[HIGHSCORE] Loaded highscores from LittleFS");
            } else {
                WebLog.println("[HIGHSCORE] File corrupt, re-initializing defaults");
                for (size_t i = 0; i < NUM_HIGHSCORES; ++i) {
                    highscores60[i].score = 0;
                    strlcpy(highscores60[i].name, "---", MAX_NAME_LEN);
                }
            }
        }
    } else {
        WebLog.println("[HIGHSCORE] No persistent file found, initializing defaults");
        for (size_t i = 0; i < NUM_HIGHSCORES; ++i) {
            highscores60[i].score = 0;
            strlcpy(highscores60[i].name, "---", MAX_NAME_LEN);
        }
    }

    clearDynamicHighscores();
}

void HighscoreManager::clearDynamicHighscores() {
    std::lock_guard<std::recursive_mutex> lock(mtx);
    for (size_t i = 0; i < NUM_HIGHSCORES; ++i) {
        highscoresDynamic[i].score = 0;
        strlcpy(highscoresDynamic[i].name, "---", MAX_NAME_LEN);
    }
}

void HighscoreManager::reset60sHighscore() {
    std::lock_guard<std::recursive_mutex> lock(mtx);
    for (size_t i = 0; i < NUM_HIGHSCORES; ++i) {
        highscores60[i].score = 0;
        strlcpy(highscores60[i].name, "---", MAX_NAME_LEN);
    }
    save60();
    WebLog.println("[HIGHSCORE] Successfully reset 60s highscores");
}

void HighscoreManager::save60() {
    std::lock_guard<std::recursive_mutex> lock(mtx);
    File file = LittleFS.open(HIGHSCORE_FILE_PATH, "w");
    if (file) {
        file.write(reinterpret_cast<const uint8_t*>(highscores60), sizeof(highscores60));
        file.close();
    } else {
        WebLog.println("[ERROR] Failed to write highscores to LittleFS");
    }
}

void HighscoreManager::checkAndAdd(const uint8_t newScore, const bool isDefaultTime) {
    std::lock_guard<std::recursive_mutex> lock(mtx);
    lastNewHighscoreIndex = -1;
    HighscoreEntry* const targetList = isDefaultTime ? highscores60 : highscoresDynamic;

    if (newScore <= targetList[NUM_HIGHSCORES - 1].score) {
        return;
    }

    int insertPos = static_cast<int>(NUM_HIGHSCORES - 1);
    for (size_t i = 0; i < NUM_HIGHSCORES; ++i) {
        if (newScore > targetList[i].score) {
            insertPos = static_cast<int>(i);
            break;
        }
    }

    for (int i = static_cast<int>(NUM_HIGHSCORES - 1); i > insertPos; --i) {
        targetList[i] = targetList[i - 1];
    }

    targetList[insertPos].score = newScore;
    strlcpy(targetList[insertPos].name, HIGHSCORE_PLACEHOLDER_NAME, MAX_NAME_LEN);
    lastNewHighscoreIndex = static_cast<int8_t>(insertPos);

    if (isDefaultTime) {
        save60();
    }
    WebLog.printf("[HIGHSCORE] New highscore registered at rank %d\n", insertPos + 1);
}

void HighscoreManager::updateHighscoreName(const int index, const char* const newName, const bool isDefaultTime) {
    std::lock_guard<std::recursive_mutex> lock(mtx);

    if (index < 0 || static_cast<size_t>(index) >= NUM_HIGHSCORES) {
        WebLog.printf("[ERROR] Highscore name update failed: index %d out of bounds\n", index);
        return;
    }
    if (newName == nullptr) {
        WebLog.println("[ERROR] Highscore name update failed: name pointer is null");
        return;
    }

    HighscoreEntry* const targetList = isDefaultTime ? highscores60 : highscoresDynamic;
    strlcpy(targetList[index].name, newName, MAX_NAME_LEN);

    if (isDefaultTime) {
        save60();
    }
    WebLog.printf("[HIGHSCORE] Updated name for rank %d: %s\n", index + 1, newName);
}

std::array<HighscoreEntry, NUM_HIGHSCORES> HighscoreManager::getHighscoresCopy(const bool isDefaultTime) const {
    std::lock_guard<std::recursive_mutex> lock(mtx);
    std::array<HighscoreEntry, NUM_HIGHSCORES> copy{};
    const HighscoreEntry* const source = isDefaultTime ? highscores60 : highscoresDynamic;

    for (size_t i = 0; i < NUM_HIGHSCORES; ++i) {
        copy[i] = source[i];
    }
    return copy;
}
