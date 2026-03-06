#ifndef _LED_DRIVER_H
#define _LED_DRIVER_H

#include "hardware/pio.h"
#include "ws2812.pio.h"

#define NUM_LEDS 64
#define LED_CTRL_PIN 15
#define LED_CTRL_FREQ 800000

class LED_Driver
{
    private: 
        uint32_t leds[64];
        const uint8_t sm;
        const PIO pio;
        uint offset;

    public:
        LED_Driver();

        void init();

        inline void put_pixel(uint32_t pixel_grb);

        uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b);

        void set_led_color(int index, uint8_t r, uint8_t g, uint8_t b);

        void show();

        void clear();
};

#endif