#include "leds.h"
#include "WebLog.h"

void LED_Driver::init()
{
    #if NUM_ROWS >= 1
        FastLED.addLeds<WS2812B, LED_DRIVER_0_PIN, GRB>((CRGB*)physical_frame_buffer[0], NUM_COLUMNS * NUM_LEDS_PER_FIELD);
    #endif

    #if NUM_ROWS >= 2
        FastLED.addLeds<WS2812B, LED_DRIVER_1_PIN, GRB>((CRGB*)physical_frame_buffer[1], NUM_COLUMNS * NUM_LEDS_PER_FIELD);
    #endif

    #if NUM_ROWS >= 3
        FastLED.addLeds<WS2812B, LED_DRIVER_2_PIN, GRB>((CRGB*)physical_frame_buffer[2], NUM_COLUMNS * NUM_LEDS_PER_FIELD);
    #endif

    #if NUM_ROWS >= 4
        FastLED.addLeds<WS2812B, LED_DRIVER_3_PIN, GRB>((CRGB*)physical_frame_buffer[3], NUM_COLUMNS * NUM_LEDS_PER_FIELD);
    #endif

    //pull the LED_DATA_EN pin low
    pinMode(2, OUTPUT);
    digitalWrite(2, 0);

    FastLED.setBrightness(MAX_BRIGHTNESS);
    FastLED.clear();
}


bool LED_Driver::show(bool force)
{
    std::lock_guard<std::mutex> lock(led_mutex);

    if(needs_refresh || force == true)
    {
        needs_refresh = false;
        
        expand();
        FastLED.show();
        
        return true;
    }
    return false;
}


void LED_Driver::expand()
{
    for(int r = 0; r < NUM_ROWS; r++)
    {
        for(int c = 0; c < NUM_COLUMNS; c++)
        {
            fill_solid(physical_frame_buffer[r][c], NUM_LEDS_PER_FIELD, frame_buffer[r][c]);
        }
    }
}


void LED_Driver::set_rgb(uint32_t hex, uint8_t index)
{
    uint8_t row = index / NUM_COLUMNS;
    uint8_t column = index % NUM_COLUMNS;
    set_rgb(hex, row, column);
}


void LED_Driver::set_rgb(uint32_t hex, uint8_t row, uint8_t column)
{
    if (row >= NUM_ROWS || column >= NUM_COLUMNS)
    {
        WebLog.printf("[LED] [WARNING] Attempted out-of-bounds write: Row %u, Col %u\n", row, column);
        return;
    }
    std::lock_guard<std::mutex> lock(led_mutex);
    needs_refresh = true;
    frame_buffer[row][column] = CRGB(hex);
}