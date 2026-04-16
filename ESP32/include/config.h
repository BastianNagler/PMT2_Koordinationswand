#pragma once

/* 
##########################
    DISCLAIMER:
Do not use GPIOs:
19 (USB D-)
20 (USB D+)
35 (PSRAM)
36 (PSRAM)
37 (PSRAM)
43 (USB Serial)
44 (USB Serial)
##########################
 */



// ---- DEVELOPMENT CONFIGURATION ----



#define NUM_FIELDS 1


// --- LEDs ---
#define NUM_LEDS_PER_FIELD 16
#define NUM_LEDS (NUM_FIELDS * NUM_LEDS_PER_FIELD)

#define WS2812B_DATA_PIN 10 // Change to actual pin
#define MAX_BRIGHTNESS 255 // Adjust brightness as needed, 0-255
// #define FastLED_USE_I2S // define for FastLED to use I2S: Problems with certain colors

// --- IO-Expander ---
#define USE_IO_EXPANDER

#ifdef USE_IO_EXPANDER
    #define NUM_IO_EXPANDER 4
    #define NUM_IO_PER_EXPANDER 16
    #define I2C_SCL_PIN 9 // Change to actual pin
    #define I2C_SDA_PIN 8 // Change to actual pin
    #define IO_EXPANDER_0_IRQ_PIN 4 // Change to actual pins
    #define IO_EXPANDER_1_IRQ_PIN 5
    #define IO_EXPANDER_2_IRQ_PIN 13
    #define IO_EXPANDER_3_IRQ_PIN 14
    #define ACTIVE_LOW_TOUCH_SENSORS false // Set to true if touch sensors are active low
#else
    #define BUTTON_PINS {4,5,6,7} // manually have to increase this if more fields are added
    #define ACTIVE_LOW_BUTTONS false // Set to true if buttons are active low
#endif
