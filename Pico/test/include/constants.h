#ifndef _CONSTANTS_H
#define _CONSTANTS_H


// ledDriver0 Defines 
#define ledDriver0_NUM_FIELDS 64
#define ledDriver0_NUM_LEDS_PER_FIELD 10
#define ledDriver0_NUM_LEDS (ledDriver0_NUM_FIELDS * ledDriver0_NUM_LEDS_PER_FIELD)
#define ledDriver0_LED_CTRL_PIN 15
#define ledDriver0_LED_CTRL_FREQ 800000

// tca9555Driver0 Defines
#define tca9555Driver0_I2C_PORT i2c1
#define tca9555Driver0_NUM_TCA9555 4
#define tca9555Driver0_I2C_SDA_PIN 2
#define tca9555Driver0_I2C_SCL_PIN 3
#define tca9555Driver0__ADDRESSES (const uint8_t[]){0x20, 0x21, 0x22, 0x23}
#define tca9555Driver0_IRQ_PINS (const uint8_t[]){6, 7, 8, 9}


#endif