#ifndef _TCA9555_DRIVER_H
#define _TCA9555_DRIVER_H

// TCA9555 specific defines
#define REG_INPUT_PORT0  0x00
#define REG_CONFIG_PORT0 0x06

class TCA9555_Driver
{
    private:
        i2c_inst_t* i2c_port;
        uint8_t num_tca9555;
        uint8_t i2c_scl_pin;
        uint8_t i2c_sda_pin;



        // TCA9555 I²C Addresses are [0 1 0 0 A2 A1 A0]
        uint8_t* tca9555_addresses;

        uint8_t* tca9555_irq_gpio_pins;

        static TCA9555_Driver* instance;

    public:
        
        volatile bool* tca9555_needs_read;



        TCA9555_Driver(i2c_inst_t* i2c_port, uint8_t num_tca9555, uint8_t i2c_scl_pin, uint8_t i2c_sda_pin, const uint8_t* addresses, const uint8_t* irq_pins);
        ~TCA9555_Driver();

        void init();

        static void tca9555_irq_callback(uint gpio, uint32_t events);

        uint16_t tca9555_i2c_readState(uint8_t idx);
};

#endif