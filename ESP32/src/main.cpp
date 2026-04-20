#include <Arduino.h>
#include <Wire.h>
#include <esp_timer.h>

#include "config.h"
#include "leds.h"

#ifdef USE_IO_EXPANDER
    #include "MCP23017.h"
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



#ifdef USE_IO_EXPANDER
    MCP23017 expander0(0x20, IO_EXPANDER_0_IRQ_PIN);
    MCP23017 expander1(0x21, IO_EXPANDER_1_IRQ_PIN);
    MCP23017 expander2(0x22, IO_EXPANDER_2_IRQ_PIN);
    MCP23017 expander3(0x23, IO_EXPANDER_3_IRQ_PIN);
    MCP23017 expanders[NUM_IO_EXPANDER] = {expander0, expander1, expander2, expander3};
#else
    uint8_t buttonPins[NUM_FIELDS] = BUTTON_PINS;
#endif

volatile bool isPressed[NUM_FIELDS];
enum GameState { IDLE, PLAYING, GAME_OVER };
GameState gameState = IDLE;

uint8_t score = 0;
uint32_t gameStartTime = 0;
uint32_t lastBlinkTime = 0;
uint8_t help_blink = 0;

uint8_t currentTargetField = 99;
uint8_t lastField = 99;

LED_Driver leds;

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
    leds.init();
}
void read(){
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
}

uint8_t getRandomGenerator(uint8_t min, uint8_t max){
    return esp_random() % (max-min) + min;
}
void setup_next_level() {
    //do {
        currentTargetField = getRandomGenerator(0,NUM_FIELDS);
    //} while (currentTargetField == lastField);
    
    lastField = currentTargetField;
    leds.set_rgb(CYAN, currentTargetField);
}

void runGameLogic(uint32_t currentTime) {
    switch (gameState) {
        
        case IDLE:
            if (currentTime - lastBlinkTime >= 500) {
                lastBlinkTime = currentTime;
                help_blink ^= 1;
                
                for (int i = 0; i < NUM_FIELDS; i++) {
                    leds.set_rgb(OFF, i);
                }
                
                if (help_blink == 1) {
                    leds.set_rgb(YELLOW, 0);
                    leds.set_rgb(BLUE,1);
                }
            }

            // Startbedingung prüfen
            if (isPressed[0] || isPressed[1]) {
                score = 0;
                lastField = 99; // Reset
                gameStartTime = currentTime;
                
                for (int i = 0; i < NUM_FIELDS; i++) leds.set_rgb(OFF, i); // Alle LEDs aus
                setup_next_level(); // Erstes Ziel generieren
                
                gameState = PLAYING;
            }
            break;

        case PLAYING:
            if (currentTime - gameStartTime >= 60000) {
                // Zeit abgelaufen!
                gameState = GAME_OVER;
            } else {
                // Warten auf Tastendruck des korrekten Feldes
                if (isPressed[currentTargetField]) {
                    score++;
                    leds.set_rgb(OFF, currentTargetField);
                    setup_next_level(); // Nächstes Ziel generieren
                }
            }
            break;

        case GAME_OVER:
            Serial.printf("Zeit abgelaufen! Dein Score ist: %d\n", score);
            for (int i = 0; i < NUM_FIELDS; i++) leds.set_rgb(OFF, i); // Alles aus
            
            lastBlinkTime = currentTime; 
            gameState = IDLE;            
            break;
    }
}


void loop()
{
    read();
    runGameLogic(millis());
    leds.show();

    delay(20);
}