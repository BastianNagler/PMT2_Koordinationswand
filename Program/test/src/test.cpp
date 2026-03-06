#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/pio.h"
#include "ws2812.pio.h"

#include "constants.h"
#include "LED_Driver.h"
#include "TCA9555_Driver.h"

TCA9555_Driver input_driver0(tca9555Driver0_I2C_PORT, tca9555Driver0_NUM_TCA9555, tca9555Driver0_I2C_SCL_PIN, tca9555Driver0_I2C_SDA_PIN,
    tca9555Driver0__ADDRESSES, tca9555Driver0_IRQ_PINS);

LED_Driver output_driver0(ledDriver0_NUM_FIELDS, ledDriver0_NUM_LEDS_PER_FIELD, ledDriver0_LED_CTRL_PIN, ledDriver0_LED_CTRL_FREQ);

int main()
{
    stdio_init_all();
    
    input_driver0.init();

    while(true) {
        for(int i = 0; i < tca9555Driver0_NUM_TCA9555; i++) {
            if(input_driver0.tca9555_needs_read[i]) {
                input_driver0.tca9555_needs_read[i] = false;
                
                uint16_t state = input_driver0.tca9555_i2c_readState(i);
                
                if (state != 0xFFFF) {
                    // Alles okay, Tasten verarbeiten!
                } else {
                    // FEHLER! Chip antwortet nicht. I2C Bus reparieren oder Warnung anzeigen.
                }
            }
        }
        sleep_ms(1);
    }
}



