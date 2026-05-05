#include <Arduino.h>
#include <Wire.h>
#include <esp_timer.h>

#include "TwallGame.h"
#include "config.h"
#include "leds.h"
#include "ioExpander.h"
#include "heartbeat.h"

const uint8_t io_exp_irq_pins[NUM_IO_EXPANDER] = IO_EXPANDER_IRQ_PINS;
const uint8_t io_exp_addresses[NUM_IO_EXPANDER] = IO_EXPANDER_ADDRESSES;


IO_Expander expanders(NUM_IO_EXPANDER, io_exp_addresses, io_exp_irq_pins);
volatile bool isPressed[NUM_FIELDS];
LED_Driver leds;
HeartbeatLED heartbeat;

void setup()
{
    Serial.begin(115200);
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN, 400000);

    loadHighscores();
    if (!expanders.init())
    {
        heartbeat.setError();
    }
    leds.init();
    heartbeat.init();
}

void loop()
{
    if(!expanders.read(isPressed, NUM_FIELDS))
    {
        heartbeat.setError();
    }
    
    runGameLogic(millis());
    leds.show();
    heartbeat.update();
}