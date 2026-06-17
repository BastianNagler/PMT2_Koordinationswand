#pragma once


// ###################
//     SOFTWARE
// ###################

#define NUM_ROWS 4 // MAX 4
#define NUM_COLUMNS 8
#define NUM_FIELDS (NUM_ROWS * NUM_COLUMNS)

#define MAX_BRIGHTNESS 255 // Adjust brightness as needed, 0-255
#define NUM_LEDS_PER_FIELD 16
#define NUM_LEDS (NUM_FIELDS * NUM_LEDS_PER_FIELD)

#define NUM_IO_EXPANDER 2
#define NUM_IO_PER_EXPANDER 16



// ###################
//     HARDWARE
// ###################

#define LED_DRIVER_0_PIN 42
#define LED_DRIVER_1_PIN 41
#define LED_DRIVER_2_PIN 40
#define LED_DRIVER_3_PIN 39

#define HEARTBEAT_LED_PIN 38

#define I2C_SCL_PIN 9
#define I2C_SDA_PIN 8
#define IO_EXPANDER_ADDRESSES {0x20, 0x21} // Change to actual addresses
#define IO_EXPANDER_IRQ_PINS {10, 14} // Change to actual pins
#define IO_EXPANDER_RESET_PIN 12

