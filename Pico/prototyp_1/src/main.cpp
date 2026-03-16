#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/pio.h"

#include "constants.h"
#include "RGB_Color.h"
#include "LED_Driver.h"

// TTP223 PIN: GPIO 10
// WS2812 PIN: GPIO 15

LED_Driver led_driver0(ledDriver0_NUM_FIELDS, ledDriver0_NUM_LEDS_PER_FIELD, ledDriver0_LED_CTRL_PIN, ledDriver0_LED_CTRL_FREQ);

int main()
{
    stdio_init_all();
    led_driver0.init();

    gpio_init(button0_PIN);
    gpio_pull_down(button0_PIN);
    gpio_set_dir(button0_PIN, GPIO_IN);

    while(true) 
    {
        if(gpio_get(button0_PIN))
        {
            RGB_Color white(255, 255, 255);
            led_driver0.set_field(0, white);
        }
        else
        {
            RGB_Color black(0, 0, 0);
            led_driver0.set_field(0, black);
        }

        led_driver0.show();

        sleep_ms(10);
        
    }
}