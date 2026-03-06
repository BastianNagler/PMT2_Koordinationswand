#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

#include "TCA9555_Driver.h"

const uint8_t TCA9555_Driver::TCA9555_ADDRESSES[4] = {0x20, 0x21, 0x22, 0x23};
const uint TCA9555_Driver::TCA9555_IRQ_GPIO_PINS[4] = {6, 7, 8, 9};
volatile bool TCA9555_Driver::tca9555_needs_read[4] = {false, false, false, false};

void TCA9555_Driver::init()
{
    // I²C Initialisation at 400Khz.
    i2c_init(I2C_PORT, 400*1000);    
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);

    for (int i = 0; i < NUM_TCA9555; i++) 
    {
        // Configure TCA9555 Pins as Inputs
        uint8_t config_cmd[3] = {REG_CONFIG_PORT0, 0xFF, 0xFF};
        //i2c_write_blocking(I2C_PORT, TCA9555_ADDRESSES[i], config_cmd, 3, false);

        // Set corresponding GPIO-Pin to IRQ 
        gpio_init(TCA9555_IRQ_GPIO_PINS[i]);
        gpio_set_dir(TCA9555_IRQ_GPIO_PINS[i], GPIO_IN);
        if(i==0)
            gpio_set_irq_enabled_with_callback(TCA9555_IRQ_GPIO_PINS[i], GPIO_IRQ_EDGE_FALL, true, &TCA9555_Driver::tca9555_irq_callback);
        else
            gpio_set_irq_enabled(TCA9555_IRQ_GPIO_PINS[i], GPIO_IRQ_EDGE_FALL, true);
    }
}

void TCA9555_Driver::tca9555_irq_callback(uint gpio, uint32_t events)
{
    for(int i = 0; i < NUM_TCA9555; i++ )
    {
        if(gpio == TCA9555_IRQ_GPIO_PINS[i])
        {
            tca9555_needs_read[i] = true;
            break;
        }
    }
}

uint16_t TCA9555_Driver::tca9555_i2c_readState(uint8_t idx) 
{
    uint8_t reg = REG_INPUT_PORT0; 
    uint8_t data[2] = {0 , 0};

    // Declare TCA9555-Register to read
    i2c_write_blocking(I2C_PORT, TCA9555_ADDRESSES[idx], &reg, 1, true); 
    
    // Read TCA9555-Register
    i2c_read_blocking(I2C_PORT, TCA9555_ADDRESSES[idx], data, 2, false);

    return (data[1] << 8) | data[0];
}