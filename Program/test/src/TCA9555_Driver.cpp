#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

#include "TCA9555_Driver.h"

TCA9555_Driver* TCA9555_Driver::instance = nullptr;

TCA9555_Driver::TCA9555_Driver(i2c_inst_t* i2c_port, uint8_t num_tca9555, uint8_t i2c_scl_pin, uint8_t i2c_sda_pin, const uint8_t* addresses, const uint8_t* irq_pins) 
    : i2c_port(i2c_port), num_tca9555(num_tca9555), i2c_scl_pin(i2c_scl_pin), i2c_sda_pin(i2c_sda_pin) 
{
    instance = this;

    tca9555_addresses = new uint8_t[num_tca9555];
    tca9555_irq_gpio_pins = new uint8_t[num_tca9555];
    tca9555_needs_read = new bool[num_tca9555];

    for(int i = 0; i < num_tca9555; i++) {
        tca9555_addresses[i] = addresses[i];
        tca9555_irq_gpio_pins[i] = irq_pins[i];
        tca9555_needs_read[i] = false;
    }
}

TCA9555_Driver::~TCA9555_Driver()
{
    delete[] tca9555_addresses;
    delete[] tca9555_irq_gpio_pins;
    delete[] tca9555_needs_read;
}


void TCA9555_Driver::init()
{
    // I²C Initialisation at 400Khz.
    i2c_init(i2c_port, 400*1000);    
    gpio_set_function(i2c_sda_pin, GPIO_FUNC_I2C);
    gpio_set_function(i2c_scl_pin, GPIO_FUNC_I2C);

    for (int i = 0; i < num_tca9555; i++) 
    {
        // Configure TCA9555 Pins as Inputs
        uint8_t config_cmd[3] = {REG_CONFIG_PORT0, 0xFF, 0xFF};
        //i2c_write_blocking(I2C_PORT, tca9555_addresses[i], config_cmd, 3, false);

        // Set corresponding GPIO-Pin to IRQ 
        gpio_init(tca9555_irq_gpio_pins[i]);
        gpio_set_dir(tca9555_irq_gpio_pins[i], GPIO_IN);
        if(i==0)
            gpio_set_irq_enabled_with_callback(tca9555_irq_gpio_pins[i], GPIO_IRQ_EDGE_FALL, true, &TCA9555_Driver::tca9555_irq_callback);
        else
            gpio_set_irq_enabled(tca9555_irq_gpio_pins[i], GPIO_IRQ_EDGE_FALL, true);
    }
}

void TCA9555_Driver::tca9555_irq_callback(uint gpio, uint32_t events)
{
    if(instance == nullptr) return;

    for(int i = 0; i < instance->num_tca9555; i++ )
    {
        if(gpio == instance->tca9555_irq_gpio_pins[i])
        {
            instance->tca9555_needs_read[i] = true;
            break;
        }
    }
}

uint16_t TCA9555_Driver::tca9555_i2c_readState(uint8_t idx) 
{
    uint8_t reg = REG_INPUT_PORT0; 
    uint8_t data[2] = {0 , 0};

    // Declare TCA9555-Register to read
    i2c_write_blocking(i2c_port, tca9555_addresses[idx], &reg, 1, true); 
    
    // Read TCA9555-Register
    i2c_read_blocking(i2c_port, tca9555_addresses[idx], data, 2, false);

    return (data[1] << 8) | data[0];
}