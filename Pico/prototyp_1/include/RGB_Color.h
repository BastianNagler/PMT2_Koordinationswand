#ifndef _COLOR_CLASS_H
#define _COLOR_CLASS_H

#include "pico/stdlib.h"

class RGB_Color{
    public:
        uint8_t red;
        uint8_t green;
        uint8_t blue;

        RGB_Color();
        RGB_Color(uint8_t red, uint8_t green, uint8_t blue);

        uint32_t RGB_To_WS2812();
};

#endif 