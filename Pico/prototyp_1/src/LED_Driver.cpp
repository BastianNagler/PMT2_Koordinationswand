#include "pico/stdlib.h"
#include "LED_Driver.h"

LED_Driver::LED_Driver(uint8_t num_fields, uint8_t num_leds_per_field, uint8_t led_ctrl_pin, uint32_t led_ctrl_freq)
    : num_fields(num_fields), num_leds_per_field(num_leds_per_field), num_leds(num_fields * num_leds_per_field), led_ctrl_pin(led_ctrl_pin), led_ctrl_freq(led_ctrl_freq),
      pio(pio0), sm(0), offset(0)
{
    leds = new RGB_Color[num_leds];
    for(int i = 0; i < num_leds; i++) {
        leds[i] = RGB_Color(0,0,0);
    }
}

LED_Driver::~LED_Driver() {
    delete[] leds;
}

void LED_Driver::init()
{
    offset = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, sm, offset, led_ctrl_pin, led_ctrl_freq, false);
}

void LED_Driver::put_pixel(RGB_Color& pixel) 
{
    pio_sm_put_blocking(pio, sm, pixel.RGB_To_WS2812());
}

void LED_Driver::set_led_color(int index, RGB_Color& color) 
{
    if (index < num_leds)
    {
        leds[index] = color;
    }
}

void LED_Driver::set_led_in_field(int field_index, int local_led_index, RGB_Color& color) 
{
    if (field_index >= 0 && field_index < num_fields && local_led_index >= 0 && local_led_index < num_leds_per_field)
    {
        int absolute_index = (field_index * num_leds_per_field) + local_led_index;
        set_led_color(absolute_index, color);
    }
}

void LED_Driver::set_field(int field_index, RGB_Color& color)
{
    for(int i = 0; i < num_leds_per_field; i++){
        set_led_in_field(field_index, i, color);
    }
}

void LED_Driver::show()
{
    for(int i = 0; i < num_leds; i++)
    {
        put_pixel(leds[i]);
    }
    // short pause after sending
    sleep_us(60);
}

void LED_Driver::clear() {
    for(int i = 0; i < num_leds; i++) {
        leds[i] = RGB_Color(0,0,0);
    }
    show();
}