#include "RGB_Color.h"

RGB_Color::RGB_Color(): red(0), green(0), blue(0)
{}

RGB_Color::RGB_Color(uint8_t red, uint8_t green, uint8_t blue) 
    : red(red), green(green), blue(blue) 
{}

uint32_t RGB_Color::RGB_To_WS2812() 
{
    return ((uint32_t)(red) << 16) | ((uint32_t)(green) << 24) | ((uint32_t)(blue) << 8);
}

