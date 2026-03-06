#include "pico/stdlib.h"
#include "LED_Driver.h"

LED_Driver::LED_Driver(): pio(pio0), sm(0), offset(0)
{
    for(int i = 0; i < NUM_LEDS; i++)
    {
        leds[i] = 0;
    }
}

void LED_Driver::init()
{
    offset = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, sm, offset, LED_CTRL_PIN, LED_CTRL_FREQ, false);
}

void LED_Driver::put_pixel(uint32_t pixel_grb) 
{
    pio_sm_put_blocking(pio, sm, pixel_grb << 8u);
}

uint32_t LED_Driver::urgb_u32(uint8_t r, uint8_t g, uint8_t b) 
{
    return ((uint32_t)(r) << 8) | ((uint32_t)(g) << 16) | (uint32_t)(b);
}

void LED_Driver::set_led_color(int index, uint8_t r, uint8_t g, uint8_t b) 
{
    if (index < NUM_LEDS)
    {
        leds[index] = urgb_u32(r, g, b);
    }
}

void LED_Driver::show()
{
    for(int i = 0; i < NUM_LEDS; i++)
    {
        put_pixel(leds[i]);
    }
    // short pause after sending
    sleep_us(60);
}

void LED_Driver::clear() {
    for(int i = 0; i < NUM_LEDS; i++) {
        leds[i] = 0;
    }
    show();
}