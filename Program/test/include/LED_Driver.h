#ifndef _LED_DRIVER_H
#define _LED_DRIVER_H

#include "hardware/pio.h"
#include "ws2812.pio.h"
#include "RGB_Color.h"

#define NUM_FIELDS 64
#define NUM_LEDS_PER_FIELD 10
#define NUM_LEDS (NUM_FIELDS * NUM_LEDS_PER_FIELD)
#define LED_CTRL_PIN 15
#define LED_CTRL_FREQ 800000

class LED_Driver
{
    private: 
        RGB_Color leds[NUM_LEDS];
        const uint8_t sm;
        const PIO pio;
        uint offset;

    public:
        LED_Driver();

        void init();

        inline void put_pixel(RGB_Color& pixel);

        void set_led_color(int index, RGB_Color& color);

        void set_led_in_field(int field_index, int local_led_index, RGB_Color& color);

        void set_field(int field_index, RGB_Color& color);

        void show();

        void clear();
};

#endif