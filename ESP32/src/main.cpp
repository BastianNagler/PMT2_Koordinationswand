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

// --- NEU: Spielmodus ---
enum GameMode { SINGLE_PLAYER, MULTI_PLAYER };
GameMode currentMode = SINGLE_PLAYER;

// --- Variablen für Spieler ---
uint8_t scoreP1 = 0;
uint8_t scoreP2 = 0;
uint8_t targetP1 = 99;
uint8_t targetP2 = 99;
uint8_t lastP1 = 99;
uint8_t lastP2 = 99;

uint32_t gameStartTime = 0;
uint32_t lastBlinkTime = 0;
uint8_t help_blink = 0;

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
void set_next_target(uint8_t player) {
    uint8_t minLimit, maxLimit;
    uint8_t* currentTarget;
    uint8_t* lastTarget;
    uint32_t color;

    if (currentMode == SINGLE_PLAYER) {
        // Ein Spieler: Ganzes Spielfeld
        minLimit = 0; 
        maxLimit = NUM_FIELDS;
        currentTarget = &targetP1;
        lastTarget = &lastP1;
        color = CYAN;
    } else { // 2 SPIELER MODUS
        if (player == 1) {
            // Player 1: Erste Hälfte
            minLimit = 0; 
            maxLimit = NUM_FIELDS / 2;
            currentTarget = &targetP1;
            lastTarget = &lastP1;
            color = MAGENTA; // Eigene Farbe für P1
        } else {
            // Player 2: Zweite Hälfte
            minLimit = NUM_FIELDS / 2; 
            maxLimit = NUM_FIELDS;
            currentTarget = &targetP2;
            lastTarget = &lastP2;
            color = ORANGE; // Eigene Farbe für P2
        }
    }

    // Neues Feld auswürfeln (nicht das gleiche wie vorher)
   // do {
        *currentTarget = getRandomGenerator(minLimit, maxLimit);
    //} while (*currentTarget == *lastTarget);
    
    *lastTarget = *currentTarget;
    leds.set_rgb(color, *currentTarget);
}

void runGameLogic(uint32_t currentTime) {
    switch (gameState) {
        
        case IDLE:
            if (currentTime - lastBlinkTime >= 500) {
                lastBlinkTime = currentTime;
                help_blink ^= 1;
                
                for (int i = 0; i < NUM_FIELDS; i++) leds.set_rgb(OFF, i);
                
                if (help_blink == 1) {
                    leds.set_rgb(YELLOW, 0); // Taste für 1-Spieler
                    leds.set_rgb(BLUE, 1);   // Taste für 2-Spieler
                }
            }

            // Startbedingung prüfen
            if (isPressed[0] || isPressed[1]) {
                // Variablen zurücksetzen
                scoreP1 = 0;
                scoreP2 = 0;
                lastP1 = 99; 
                lastP2 = 99;
                gameStartTime = currentTime;
                
                for (int i = 0; i < NUM_FIELDS; i++) leds.set_rgb(OFF, i);
                
                // Modus festlegen und Felder generieren
                if (isPressed[0]) {
                    currentMode = SINGLE_PLAYER;
                    set_next_target(1); 
                } else if (isPressed[1]) {
                    currentMode = MULTI_PLAYER;
                    set_next_target(1); // Start-Ziel für P1
                    set_next_target(2); // Start-Ziel für P2
                }
                
                gameState = PLAYING;
            }
            break;

        case PLAYING:
            if (currentTime - gameStartTime >= 60000) {
                // Zeit abgelaufen!
                gameState = GAME_OVER;
            } else {
                // Player 1 Check (läuft im 1- und 2-Spieler-Modus)
                if (isPressed[targetP1]) {
                    scoreP1++;
                    leds.set_rgb(OFF, targetP1);
                    set_next_target(1);
                }
                
                // Player 2 Check (läuft NUR im 2-Spieler-Modus)
                if (currentMode == MULTI_PLAYER) {
                    if (isPressed[targetP2]) {
                        scoreP2++;
                        leds.set_rgb(OFF, targetP2);
                        set_next_target(2);
                    }
                }
            }
            break;

        case GAME_OVER:
            if (currentMode == SINGLE_PLAYER) {
                Serial.printf("Zeit abgelaufen! Score: %d\n", scoreP1);
            } else {
                Serial.printf("Zeit abgelaufen! P1: %d | P2: %d\n", scoreP1, scoreP2);
            }
            
            for (int i = 0; i < NUM_FIELDS; i++) leds.set_rgb(OFF, i); 
            
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