#pragma once
#include <Arduino.h>
#include <FastLED.h>
#include "config.h"

/*
    Heartbeat LED class to indicate system status.
    - Green: Normal operation (pulsing organically)
    - Red: Error detected (pulsing organically)
*/

class HeartbeatLED
{
private:
    CRGB led;
    bool hasError = false;

public:
    /// @brief Initializes the Heartbeat LED
    void init();

    /// @brief Updates the Heartbeat LED based on system status
    void update();

    /// @brief Sets error state to true
    void setError();
};

