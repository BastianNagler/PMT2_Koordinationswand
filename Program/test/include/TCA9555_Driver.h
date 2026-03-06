#ifndef _TCA9555_DRIVER_H
#define _TCA9555_DRIVER_H

// I2C defines
#define I2C_PORT i2c1
#define I2C_SDA_PIN 2
#define I2C_SCL_PIN 3
#define NUM_TCA9555 4

// TCA9555 specific defines
#define REG_INPUT_PORT0  0x00
#define REG_CONFIG_PORT0 0x06

class TCA9555_Driver
{
    private:
        // TCA9555 I²C Addresses are [0 1 0 0 A2 A1 A0]
        static const uint8_t TCA9555_ADDRESSES[4];

        static const uint TCA9555_IRQ_GPIO_PINS[4];

    public:
        
        static volatile bool tca9555_needs_read[4];




        void init();

        static void tca9555_irq_callback(uint gpio, uint32_t events);

        uint16_t tca9555_i2c_readState(uint8_t idx);
};

#endif