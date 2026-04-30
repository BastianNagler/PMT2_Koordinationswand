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
#define NUM_COLUMNS 4

#define NUM_FIELDS (NUM_ROWS * NUM_COLUMNS)


// --- LEDs ---
#define NUM_LEDS_PER_FIELD 16
#define NUM_LEDS (NUM_FIELDS * NUM_LEDS_PER_FIELD)

#define LED_DRIVER_0_PIN 6 // Change to actual pin
#define LED_DRIVER_1_PIN 7 // Change to actual pin
#define LED_DRIVER_2_PIN 15 // Change to actual pin
#define LED_DRIVER_3_PIN 16 // Change to actual pin

#define MAX_BRIGHTNESS 255 // Adjust brightness as needed, 0-255


// --- IO-Expander ---
#define NUM_IO_EXPANDER 1
#define NUM_IO_PER_EXPANDER 2
#define I2C_SCL_PIN 41 // Change to actual pin
#define I2C_SDA_PIN 40 // Change to actual pin
#define IO_EXPANDER_ADDRESSES {0x20} // Change to actual addresses
#define IO_EXPANDER_IRQ_PINS {10} // Change to actual pins


// --- Screen ---
#define I2C_SCREEN_0 21
#define I2C_SCREEN_1 47
