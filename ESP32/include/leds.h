#pragma once
#include <Arduino.h>
#include <FastLED.h>
#include "config.h"

/*
convention for running-indexes of fields: 
    column  |   0   1   2   3   4   5   6   7   
row         |
--------------------------------------------------------
    0       |   0   1   2   3   4   5   6   7
    1       |   8   9   10  11  12  13  14  15
    2       |   16  17  18  19  20  21  22  23
    3       |   24  25  26  27  28  29  30  31
*/

class LED_Driver
{
private: 
    CRGB frame_buffer[NUM_ROWS][NUM_COLUMNS];
    CRGB physical_frame_buffer[NUM_ROWS][NUM_COLUMNS][NUM_LEDS_PER_FIELD];
    volatile bool needs_refresh = true;

    /// @brief expands the frame_buffer to the physical_frame_buffer 
    void expand();

public: 
    /// @brief initializes FastLED for 4 drivers
    void init();

    /// @brief refreshes the LEDs if something changed
    /// @param force if true, always refreshes the LEDs, even without change
    /// @return true if a refresh happened, false if none happened
    bool show(bool force = false);

    /// @brief set field to specified hex 
    /// @param hex hex-value for color
    /// @param index index of field to be set
    void set_rgb(uint32_t hex, uint8_t index);

    /// @brief set field to specified hex
    /// @param hex hex-value for color
    /// @param row row-index for field to be set
    /// @param column column-index for field to be set
    void set_rgb(uint32_t hex, uint8_t row, uint8_t column);
};