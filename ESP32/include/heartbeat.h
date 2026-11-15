#pragma once

#include <Arduino.h>
#include <FastLED.h>
#include <WS2812Write.h>
#include <atomic>
#include <cstdint>
#include "config.h"

/**
 * Controller for the onboard WS2812 status LED displaying system health.
 *
 * Visual indications:
 * - Green organic pulse: Normal FreeRTOS multi-task operation.
 * - Red organic pulse: Error condition detected (bus fault, I/O failure).
 */
class HeartbeatLED {
private:
    std::atomic<bool> hasError{false};

public:
    HeartbeatLED() = default;
    ~HeartbeatLED() = default;
    HeartbeatLED(const HeartbeatLED&) = delete;
    HeartbeatLED& operator=(const HeartbeatLED&) = delete;

    /**
     * Calculates sinusoidal pulse brightness and updates the onboard WS2812 status LED.
     */
    void update();

    /**
     * Flags the system as having encountered a critical hardware or bus error.
     */
    void setError();

    /**
     * Queries whether an error state is currently latched.
     *
     * @return `true` if error state is active, `false` otherwise.
     */
    bool getError() const {
        return hasError.load();
    }
};
