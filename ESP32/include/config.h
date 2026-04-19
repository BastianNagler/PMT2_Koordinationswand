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

#define NUM_ROWS 1 // MAX 4
#define NUM_COLUMNS 1

#define NUM_FIELDS (NUM_ROWS * NUM_COLUMNS)


// --- LEDs ---
#define NUM_LEDS_PER_FIELD 16
#define NUM_LEDS (NUM_FIELDS * NUM_LEDS_PER_FIELD)

#define NUM_LED_DRIVERS 4 // DO NOT CHANGE
#define LED_DRIVER_0_PIN 42 // Change to actual pin
#define LED_DRIVER_1_PIN 41 // Change to actual pin
#define LED_DRIVER_2_PIN 40 // Change to actual pin
#define LED_DRIVER_3_PIN 39 // Change to actual pin

#define MAX_BRIGHTNESS 255 // Adjust brightness as needed, 0-255

// --- IO-Expander ---
// #define USE_IO_EXPANDER

#ifdef USE_IO_EXPANDER
    #define NUM_IO_EXPANDER 4 // DO NOT CHANGE
    #define NUM_IO_PER_EXPANDER 16
    #define I2C_SCL_PIN 9 // Change to actual pin
    #define I2C_SDA_PIN 8 // Change to actual pin
    #define IO_EXPANDER_0_IRQ_PIN 4 // Change to actual pins
    #define IO_EXPANDER_1_IRQ_PIN 5
    #define IO_EXPANDER_2_IRQ_PIN 13
    #define IO_EXPANDER_3_IRQ_PIN 14
    #define ACTIVE_LOW_TOUCH_SENSORS false // Set to true if touch sensors are active low
#else
    #define BUTTON_PINS {4} // manually have to increase this if more fields are added
    #define ACTIVE_LOW_BUTTONS false // Set to true if buttons are active low
#endif
