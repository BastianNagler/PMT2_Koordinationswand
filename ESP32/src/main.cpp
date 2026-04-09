#include <Arduino.h>
#include "config.h"

#ifdef USE_IO_EXPANDER
    #include <ESP_IOExpander.h>
    #include <ESP_IOExpander_Library.h>
#endif

#define FASTLED_ESP32_I2S
#define FASTLED_USES_ESP32S3_I2S
#include <FastLED.h>


const CRGB fsei = CRGB(0x3faecc);


#ifdef USE_IO_EXPANDER
    volatile bool needsRead[NUM_IO_EXPANDER];
    uint8_t interruptPins[NUM_IO_EXPANDER] = IO_EXPANDER_IRQ_PINS;

    ESP_IOExpander *expander0 = new ESP_IOExpander_TCA95xx_16bit(I2C_NUM_1, ESP_IO_EXPANDER_I2C_TCA9555_ADDRESS_000, I2C_SCL_PIN, I2C_SDA_PIN);
    ESP_IOExpander *expander1 = new ESP_IOExpander_TCA95xx_16bit(I2C_NUM_1, ESP_IO_EXPANDER_I2C_TCA9555_ADDRESS_001, I2C_SCL_PIN, I2C_SDA_PIN);
    ESP_IOExpander *expander2 = new ESP_IOExpander_TCA95xx_16bit(I2C_NUM_1, ESP_IO_EXPANDER_I2C_TCA9555_ADDRESS_010, I2C_SCL_PIN, I2C_SDA_PIN);
    ESP_IOExpander *expander3 = new ESP_IOExpander_TCA95xx_16bit(I2C_NUM_1, ESP_IO_EXPANDER_I2C_TCA9555_ADDRESS_011, I2C_SCL_PIN, I2C_SDA_PIN);

    ESP_IOExpander *expanders[NUM_IO_EXPANDER] = {expander0, expander1, expander2, expander3};


    void IRAM_ATTR onInterrupt0() { needsRead[0] = true; }
    void IRAM_ATTR onInterrupt1() { needsRead[1] = true; }
    void IRAM_ATTR onInterrupt2() { needsRead[2] = true; }
    void IRAM_ATTR onInterrupt3() { needsRead[3] = true; }

    void (*interruptHandlers[NUM_IO_EXPANDER])() = {onInterrupt0, onInterrupt1, onInterrupt2, onInterrupt3};
#else
    uint8_t buttonPins[NUM_FIELDS] = BUTTON_PINS;
#endif

CRGB leds[NUM_LEDS];
volatile bool isPressed[NUM_FIELDS];


void setup()
{
    Serial.begin(115200);
    delay(10 * 1000); // Delay for 10 seconds to allow LEDs to fully power on 
    Serial.println("ESP32-S3 Setup Start..."); // TODO: Delete

    // --- Inputs ---
    #ifdef USE_IO_EXPANDER
        Serial.println("ESP32-S3 Using IO Expander..."); // TODO: Delete
        // --- IO-Expander ---
        Serial.println("ESP32-S3 Setup IO Expander..."); // TODO: Delete
        for (int i = 0; i < NUM_IO_EXPANDER; i++)
        {
            delay(100);
            expanders[i]->init();
            expanders[i]->begin();
            expanders[i]->multiPinMode(0xFFFF, INPUT); // Set all pins as input
        }

        // --- IO-Expander Interrupts ---
        Serial.println("ESP32-S3 Setup IRQ..."); // TODO: Delete
        for (int i = 0; i < NUM_IO_EXPANDER; i++)
        {
            pinMode(interruptPins[i], INPUT_PULLUP); //TODO: Change to no PullUp when using external pull-ups
            attachInterrupt(digitalPinToInterrupt(interruptPins[i]), interruptHandlers[i], FALLING);
            needsRead[i] = true; // Force initial read of all IO-Expanders
        }
    #else
        Serial.println("ESP32-S3 Using Direct GPIO..."); // TODO: Delete
        // --- Direct GPIO ---
        Serial.println("ESP32-S3 Setup Buttons..."); // TODO: Delete
        for (int i = 0; i < NUM_FIELDS; i++)
        {
            if(ACTIVE_LOW_BUTTONS)
                pinMode(buttonPins[i], INPUT_PULLUP); // Active Low with internal pull-up
            else
                pinMode(buttonPins[i], INPUT_PULLDOWN); // Active High, internal pull-down
            isPressed[i] = false;
        }
    #endif

    // --- LEDs ---
    Serial.println("ESP32-S3 Setup LEDs..."); // TODO: Delete
    
    FastLED.addLeds<WS2812B, WS2812B_DATA_PIN, GBR>(leds, NUM_LEDS);
    FastLED.setBrightness(MAX_BRIGHTNESS);

    FastLED.clear();
    FastLED.show();
}

void loop()
{
    // --- Read Inputs ---
    #ifdef USE_IO_EXPANDER
        // Read IO-Expander states if needed
        for (int i = 0; i < NUM_IO_EXPANDER; i++)
        {
            if (needsRead[i])
            {
                needsRead[i] = false;
                uint16_t pinStates = expanders[i]->multiDigitalRead(0xFFFF); // Read all pins at once
                for (int k = 0; k < NUM_IO_PER_EXPANDER; k++)
                {
                    Serial.println("ESP32-S3 Reading IO Expander " + String(i) + " Pin " + String(k)); // TODO: Delete
                    int fieldIndex = i * NUM_IO_PER_EXPANDER + k;
                    if (fieldIndex < NUM_FIELDS)
                    {
                        if (ACTIVE_LOW_TOUCH_SENSORS)
                            isPressed[fieldIndex] = (pinStates & (1 << k)) == 0; // Active Low
                        else
                            isPressed[fieldIndex] = (pinStates & (1 << k)) != 0; // Active High
                    }
                }
            }
        }
    #else
        // Read button states directly from GPIO
        for (int i = 0; i < NUM_FIELDS; i++)
        {
            if (ACTIVE_LOW_BUTTONS)
                isPressed[i] = digitalRead(buttonPins[i]) == LOW; // Active Low
            else
                isPressed[i] = digitalRead(buttonPins[i]) == HIGH; // Active High
        }
    #endif


    // --- Update LEDs ---
    // Pressed: Green, Not pressed: Red
    for (int i = 0; i < NUM_FIELDS; i++)
    {
        for(int k = 0; k < NUM_LEDS_PER_FIELD; k++)
        {
            int ledIndex = i * NUM_LEDS_PER_FIELD + k;
            if (isPressed[i])
            {
                leds[ledIndex] = CRGB(0x00FFFF); // Pressed: Green
            }
            else
            {
                leds[ledIndex] = CRGB(0xFFFF00); // Not pressed: Red
            }
        }
    }
    FastLED.show();

    delay(20);
}