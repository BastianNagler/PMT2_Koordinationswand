#ifndef _LED_DRIVER_H
#define _LED_DRIVER_H

#include <vector>
#include "hardware/pio.h"
#include "ws2812.pio.h"
#include "RGB_Color.h"

class LED_Driver
{
    private: 
        uint8_t num_fields;
        uint8_t num_leds_per_field;
        uint8_t num_leds = num_fields * num_leds_per_field;
        uint8_t led_ctrl_pin;
        uint8_t led_ctrl_freq;

        std::vector<RGB_Color> leds;
        const uint8_t sm;
        const PIO pio;
        uint offset;

    public:
        LED_Driver(uint8_t num_fields, uint8_t num_leds_per_field, uint8_t led_ctrl_pin, uint8_t led_ctrl_freq);

        void init();

        inline void put_pixel(RGB_Color& pixel);

        void set_led_color(int index, RGB_Color& color);

        void set_led_in_field(int field_index, int local_led_index, RGB_Color& color);

        void set_field(int field_index, RGB_Color& color);

        void show();

        void clear();
};

#endif