#include "leds.h"
#include <cstring>
#include "WebLog.h"

void LED_Driver::init() {
    clear();
    {
        std::lock_guard<std::mutex> lock(buffer_mutex);
        memset(front_buffer, 0, sizeof(front_buffer));
    }

#if NUM_ROWS >= 1
    FastLED.addLeds<WS2812B, LED_DRIVER_0_PIN, GRB>(
        reinterpret_cast<CRGB*>(physical_frame_buffer[0]), NUM_COLUMNS * NUM_LEDS_PER_FIELD);
#endif
#if NUM_ROWS >= 2
    FastLED.addLeds<WS2812B, LED_DRIVER_1_PIN, GRB>(
        reinterpret_cast<CRGB*>(physical_frame_buffer[1]), NUM_COLUMNS * NUM_LEDS_PER_FIELD);
#endif
#if NUM_ROWS >= 3
    FastLED.addLeds<WS2812B, LED_DRIVER_2_PIN, GRB>(
        reinterpret_cast<CRGB*>(physical_frame_buffer[2]), NUM_COLUMNS * NUM_LEDS_PER_FIELD);
#endif
#if NUM_ROWS >= 4
    FastLED.addLeds<WS2812B, LED_DRIVER_3_PIN, GRB>(
        reinterpret_cast<CRGB*>(physical_frame_buffer[3]), NUM_COLUMNS * NUM_LEDS_PER_FIELD);
#endif

    pinMode(LED_DATA_EN_PIN, OUTPUT);
    digitalWrite(LED_DATA_EN_PIN, HIGH);

    FastLED.setBrightness(MAX_BRIGHTNESS);
    FastLED.clear();
}

bool LED_Driver::show(const bool force) {
    CRGB render_buffer[NUM_ROWS][NUM_COLUMNS];
    bool should_render = false;

    {
        std::lock_guard<std::mutex> lock(buffer_mutex);
        if (needs_refresh || force) {
            memcpy(render_buffer, front_buffer, sizeof(render_buffer));
            needs_refresh = false;
            should_render = true;
        }
    }

    if (should_render) {
        expand(render_buffer);
        FastLED.show();
        return true;
    }
    return false;
}

void LED_Driver::commitFrame() {
    std::lock_guard<std::mutex> lock(buffer_mutex);
    memcpy(front_buffer, back_buffer, sizeof(front_buffer));
    needs_refresh = true;
}

void LED_Driver::clear() {
    memset(back_buffer, 0, sizeof(back_buffer));
}

void LED_Driver::expand(const CRGB source[NUM_ROWS][NUM_COLUMNS]) {
    for (uint8_t r = 0; r < NUM_ROWS; ++r) {
        for (uint8_t c = 0; c < NUM_COLUMNS; ++c) {
            fill_solid(physical_frame_buffer[r][c], NUM_LEDS_PER_FIELD, source[r][c]);
        }
    }
}

void LED_Driver::set_rgb(const uint32_t hex, const uint8_t index) {
    if (index >= NUM_FIELDS) {
        return;
    }
    set_rgb(hex, static_cast<uint8_t>(index / NUM_COLUMNS), static_cast<uint8_t>(index % NUM_COLUMNS));
}

void LED_Driver::set_rgb(const uint32_t hex, const uint8_t row, const uint8_t column) {
    if (row >= NUM_ROWS || column >= NUM_COLUMNS) {
        return;
    }

    back_buffer[row][column] = CRGB(hex);
}
