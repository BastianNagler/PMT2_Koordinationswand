#pragma once
#include <Arduino.h>
#include <FastLED.h>
#include <WS2812Write.h>
#include "config.h"


/*
    Heartbeat LED class to indicate system status.
    - Green: Normal operation (pulsing organically)
    - Red: Error detected (pulsing organically)
*/

class HeartbeatLED
{
private:
    uint32_t color = 0xFFFF00;
    bool hasError = false;

public:
    /// @brief Updates the Heartbeat LED based on system status
    void update();

    /// @brief Sets error state to true
    void setError();
};

