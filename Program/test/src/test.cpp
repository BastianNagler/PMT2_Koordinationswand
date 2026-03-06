#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/pio.h"
#include "ws2812.pio.h"

#include "LED_Driver.h"
#include "TCA9555_Driver.h"

TCA9555_Driver input_driver;

int main()
{
    stdio_init_all();
    
    input_driver.init();

    while(true) {
        for(int i = 0; i < 4; i++) {
            if(input_driver.tca9555_needs_read[i]) {
                input_driver.tca9555_needs_read[i] = false;
                
                uint16_t state = input_driver.tca9555_i2c_readState(i);
                
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



