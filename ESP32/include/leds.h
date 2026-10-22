#pragma once

#include <Arduino.h>
#include <FastLED.h>
#include <cstdint>
#include <mutex>
#include "config.h"

/**
 * Driver managing WS2812B LED matrix across multiple parallel data pins.
 *
 * Physical layout mapping:
 * Row 0: Fields 0..7
 * Row 1: Fields 8..15
 * Row 2: Fields 16..23
 * Row 3: Fields 24..31
 */
class LED_Driver {
private:
    CRGB frame_buffer[NUM_ROWS][NUM_COLUMNS];
    CRGB physical_frame_buffer[NUM_ROWS][NUM_COLUMNS][NUM_LEDS_PER_FIELD];
    volatile bool needs_refresh = true;
    mutable std::mutex led_mutex;

    /**
     * Replicates each logical field color to its 16 physical WS2812B LEDs.
     */
    void expand();

public:
    LED_Driver() = default;
    ~LED_Driver() = default;
    LED_Driver(const LED_Driver&) = delete;
    LED_Driver& operator=(const LED_Driver&) = delete;

    /**
     * Initializes FastLED strip controllers on parallel GPIO outputs.
     */
    void init();

    /**
     * Transmits frame data to the physical LED strips if modifications occurred.
     *
     * @param[in] force When `true`, forces update even if buffer is unchanged.
     *
     * @return `true` if LED strips were updated, `false` if skipped.
     */
    bool show(const bool force = false);

    /**
     * Sets target RGB color for an individual field by its linear index (0..31).
     *
     * @param[in] hex 24-bit 0x00RRGGBB color value.
     * @param[in] index Linear index of the field.
     */
    void set_rgb(const uint32_t hex, const uint8_t index);

    /**
     * Sets target RGB color for an individual field by row and column coordinates.
     *
     * @param[in] hex 24-bit 0x00RRGGBB color value.
     * @param[in] row Grid row index (0..NUM_ROWS-1).
     * @param[in] column Grid column index (0..NUM_COLUMNS-1).
     */
    void set_rgb(const uint32_t hex, const uint8_t row, const uint8_t column);
};
