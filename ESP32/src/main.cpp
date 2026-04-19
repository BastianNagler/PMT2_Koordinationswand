#include <Arduino.h>
#include <Wire.h>
#include "config.h"
#include "esp_timer.h"

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

#define RED     0x00FF0000
#define GREEN   0x0000FF00
#define BLUE    0x000000FF
#define YELLOW  0x00FFFF00
#define CYAN    0x0000FFFF
#define MAGENTA 0x00FF00FF
#define ORANGE  0x00FFA500
#define PURPLE  0x00800080
#define WHITE   0x00FFFFFF
#define GRAY    0x00AAAAAA
#define OFF     0x00000000

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



//nur zum build!!! Basti macht das selbst
void set_rgb(uint32_t hex, uint8_t index){
    //TODO
}

uint8_t getRandomGenerator(){
    return esp_random() % NUM_FIELDS;
}

void level(uint8_t* score, uint32_t startTime){
    static uint8_t lastfield = 99;
    uint8_t nextfield;

    // Würfelt so lange neu, bis ein ANDERES Feld als beim letzten Mal herauskommt.
    do{
        nextfield = getRandomGenerator();
    } while (nextfield == lastfield);
    lastfield = nextfield;
    set_rgb(CYAN,nextfield);
    FastLED.show();
    while (isPressed[nextfield] == false){
        if (((esp_timer_get_time() / 1000) - startTime) >= 60000) {return;}
    }
    (*score) ++;
    set_rgb(OFF,nextfield);
    FastLED.show();
}
uint16_t startgame(){
    uint8_t score = 0;
    uint32_t startTime = esp_timer_get_time() / 1000;
    while ((esp_timer_get_time() / 1000) - startTime < 60000) {
        level(&score, startTime);
    }
    return score;
}

void loop()
{   
    static uint8_t help_blink = 0;
    help_blink ^= (1<<0);
    //die erste Box blinkt bis Spiel startet --> idle
    for (int i = 0; i<NUM_FIELDS; i++){
        set_rgb(OFF,i);
    }
    if(help_blink==1){set_rgb(YELLOW,0);}else{set_rgb(OFF,0)};
    FastLED.show();
    if(isPressed[0] == true || isPressed[1] == true){
        uint32_t score = startgame();
        Serial.printf("Zeit abgelaufen! Dein Score ist: %d\n", score);
    }
    delay(5000);
}