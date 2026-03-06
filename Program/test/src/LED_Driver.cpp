#include "pico/stdlib.h"
#include "LED_Driver.h"

LED_Driver::LED_Driver(): pio(pio0), sm(0), offset(0)
{}

void LED_Driver::init()
{
    offset = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, sm, offset, LED_CTRL_PIN, LED_CTRL_FREQ, false);
}

void LED_Driver::put_pixel(RGB_Color& pixel) 
{
    pio_sm_put_blocking(pio, sm, pixel.RGB_To_WS2812());
}

void LED_Driver::set_led_color(int index, RGB_Color& color) 
{
    if (index < NUM_LEDS)
    {
        leds[index] = color;
    }
}

void LED_Driver::set_led_in_field(int field_index, int local_led_index, RGB_Color& color) 
{
    if (field_index >= 0 && field_index < NUM_FIELDS && local_led_index >= 0 && local_led_index < NUM_LEDS_PER_FIELD)
    {
        int absolute_index = (field_index * NUM_LEDS_PER_FIELD) + local_led_index;
        set_led_color(absolute_index, color);
    }
}

void LED_Driver::set_field(int field_index, RGB_Color& color)
{
    for(int i = 0; i < NUM_LEDS_PER_FIELD; i++){
        set_led_in_field(field_index, i, color);
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
        leds[i] = RGB_Color(0,0,0);
    }
    show();
}