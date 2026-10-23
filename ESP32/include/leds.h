#pragma once

#include <Arduino.h>
#include <FastLED.h>
#include <cstdint>
#include <mutex>
#include "config.h"

/**
 * Driver managing WS2812B LED matrix across multiple parallel data pins.
 *
 * Implements decoupled double-buffering:
 * - Back-buffer is updated by `gameTask` without locking.
 * - Calling `commitFrame()` transfers the complete back-buffer to the front-buffer
 *   under a sub-microsecond mutex lock, guaranteeing zero frame-tearing.
 * - `show()` transfers the front-buffer and transmits over WS2812 data pins without
 *   holding the mutex during the transmission, preventing any blocking of real-time loops.
 *
 * Physical layout mapping:
 * Row 0: Fields 0..7
 * Row 1: Fields 8..15
 * Row 2: Fields 16..23
 * Row 3: Fields 24..31
 */
class LED_Driver {
private:
    CRGB back_buffer[NUM_ROWS][NUM_COLUMNS];
    CRGB front_buffer[NUM_ROWS][NUM_COLUMNS];
    CRGB physical_frame_buffer[NUM_ROWS][NUM_COLUMNS][NUM_LEDS_PER_FIELD];

    volatile bool needs_refresh = true;
    mutable std::mutex buffer_mutex;

    /**
     * Replicates logical field colors from a source buffer into the physical LED buffer.
     *
     * @param[in] source Source logical color grid.
     */
    void expand(const CRGB source[NUM_ROWS][NUM_COLUMNS]);

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
     * Transmits the committed front frame buffer to physical LED strips.
     *
     * @param[in] force When `true`, forces update even if buffer is unchanged.
     *
     * @return `true` if LED strips were updated, `false` if skipped.
     */
    bool show(const bool force = false);

    /**
     * Atomically publishes the active back-buffer to the front-buffer for rendering.
     * Must be called at the conclusion of frame generation in `gameTask`.
     */
    void commitFrame();

    /**
     * Clears all fields in the active back-buffer to black (`COLOR_OFF`).
     */
    void clear();

    /**
     * Sets target RGB color for an individual field in the back-buffer by linear index.
     *
     * @param[in] hex 24-bit 0x00RRGGBB color value.
     * @param[in] index Linear index of the field (0..NUM_FIELDS-1).
     */
    void set_rgb(const uint32_t hex, const uint8_t index);

    /**
     * Sets target RGB color for an individual field in the back-buffer by coordinates.
     *
     * @param[in] hex 24-bit 0x00RRGGBB color value.
     * @param[in] row Grid row index (0..NUM_ROWS-1).
     * @param[in] column Grid column index (0..NUM_COLUMNS-1).
     */
    void set_rgb(const uint32_t hex, const uint8_t row, const uint8_t column);
};
