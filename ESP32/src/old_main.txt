#include <Arduino.h>
#include <Wire.h>
#include "config.h"

#ifdef USE_IO_EXPANDER
    #include "MCP23017.h"
#endif

#ifdef FastLED_USE_I2S
    #define FASTLED_ESP32_I2S
    #define FASTLED_USES_ESP32S3_I2S
#endif
#include <FastLED.h>

#ifdef USE_IO_EXPANDER
    MCP23017 expander0(0x20, IO_EXPANDER_0_IRQ_PIN);
    MCP23017 expander1(0x21, IO_EXPANDER_1_IRQ_PIN);
    MCP23017 expander2(0x22, IO_EXPANDER_2_IRQ_PIN);
    MCP23017 expander3(0x23, IO_EXPANDER_3_IRQ_PIN);
    MCP23017 expanders[NUM_IO_EXPANDER] = {expander0, expander1, expander2, expander3};
#else
    uint8_t buttonPins[NUM_FIELDS] = BUTTON_PINS;
#endif

CRGB leds[NUM_LEDS];
volatile bool isPressed[NUM_FIELDS];


void setup()
{
    Serial.begin(115200);
    delay(5 * 1000); // Delay for 5 seconds to allow LEDs to fully power on 
    Serial.println("ESP32-S3 Setup Start...");

    // --- Inputs ---
    #ifdef USE_IO_EXPANDER
        Serial.println("ESP32-S3 Using IO Expander...");

        // --- IO-Expander ---
        Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
        Wire.setClock(400000);
        
        Serial.println("ESP32-S3 Setup IO Expander...");
        for (int i = 0; i < NUM_IO_EXPANDER; i++)
        {
            expanders[i].init();
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
    #ifdef FastLED_USE_I2S
        FastLED.addLeds<WS2812B, WS2812B_DATA_PIN, GBR>(leds, NUM_LEDS);
    #else
        FastLED.addLeds<WS2812B, WS2812B_DATA_PIN, GRB>(leds, NUM_LEDS);
    #endif

    FastLED.setBrightness(MAX_BRIGHTNESS);
    FastLED.setMaxPowerInVoltsAndMilliamps(5, 2500);

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
            if (expanders[i].needsRead)
            {
                uint16_t pinStates = expanders[i].read(); // Read all pins at once
                for (int k = 0; k < NUM_IO_PER_EXPANDER; k++)
                {
                    Serial.printf("ESP32-S3 Reading IO Expander %d Pin %d\n", i, k); // TODO: Delete
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
                leds[ledIndex] = CRGB::Yellow; // Pressed: Green
            }
            else
            {
                leds[ledIndex] = CRGB::Pink; // Not pressed: Red
            }
        }
    }
    FastLED.show();

    delay(20);
}