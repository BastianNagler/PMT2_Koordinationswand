#include <Arduino.h>
#include <Wire.h>
#include <esp_timer.h>
#include <esp_task_wdt.h>
#include <LittleFS.h>

#include "GameLogic.h"
#include "config.h"
#include "leds.h"
#include "ioExpander.h"
#include "heartbeat.h"
#include "WebInterface.h"
#include "WebLog.h"

volatile bool isPressed[NUM_FIELDS];
LED_Driver leds;
HeartbeatLED heartbeat;
GameLogic gameInstance(isPressed, leds);

void inputTask(void *pvParameters);
void gameTask(void *pvParameters);
void webTask(void *pvParameters);
void ledTask(void *pvParameters);
void heartbeatTask(void *pvParameters);

// --- Custom Panic Handler ---
void custom_panic_handler(arduino_panic_info_t *info, void *arg) {
    ws2812Write(HEARTBEAT_LED_PIN, 0x00FF00); 
}

void setup()
{
    delay(500);
    Serial.begin(115200);

    // Initialize LittleFS once at startup
    if (!LittleFS.begin(true))
    {
        WebLog.println("[ERROR] LittleFS mounting failed during setup!");
    }
    else
    {
        WebLog.println("[SETUP] LittleFS mounted successfully.");
    }

    // Recover the I2C bus if stuck before starting communication
    recoverI2CBus(I2C_SDA_PIN, I2C_SCL_PIN);

    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN, 400000);
    Wire.setTimeOut(100);

    set_arduino_panic_handler(custom_panic_handler, NULL);

    xTaskCreate(ledTask, "LED Task", 4096, NULL, 4, NULL);
    xTaskCreatePinnedToCore(inputTask, "Input Task", 4096, NULL, 3, NULL, 1);
    xTaskCreate(gameTask, "Game Task", 4096, NULL, 2, NULL);
    xTaskCreate(webTask, "Web Task", 8192, NULL, 1, NULL);
    xTaskCreate(heartbeatTask, "Heartbeat Task", 2048, NULL, 0, NULL);
    vTaskDelete(NULL);
}

/// @brief Task for handling hardware inputs. High priority to ensure responsiveness.
void inputTask(void *pvParameters) 
{
    static const uint8_t io_exp_irq_pins[NUM_IO_EXPANDER] = IO_EXPANDER_IRQ_PINS;
    static const uint8_t io_exp_addresses[NUM_IO_EXPANDER] = IO_EXPANDER_ADDRESSES;
    static IO_Expander expanders(NUM_IO_EXPANDER, io_exp_addresses, io_exp_irq_pins);

    if (!expanders.init())
    {
        heartbeat.setError();
    }

    /*
    dirty fix because PCB broke for Box index 16: read box over GPIO18
    if PCB gets fixed: delete following line and connect Sensor 16 back to IO-Expander
    */
    pinMode(18, INPUT);

    esp_task_wdt_add(NULL);

    while (1) {
        if (!expanders.read(isPressed, NUM_FIELDS))
        {
            WebLog.println("[ERROR] IO-Expander read failed! Resetting MCP23018s...");
            heartbeat.setError();
            if (expanders.resetAndReinit())
            {
                WebLog.println("[IO-EXPANDER] MCP23018s reset and re-initialized successfully.");
            }
            else
            {
                WebLog.println("[ERROR] Failed to re-initialize MCP23018s after reset.");
            }
        }

        /*
        dirty fix because PCB broke for Box index 16: read box over GPIO18
        if PCB gets fixed: delete following line and connect Sensor 16 back to IO-Expander
        */
        if(NUM_FIELDS >= 17)
            isPressed[16] = digitalRead(18);

        esp_task_wdt_reset();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/// @brief Task for running game logic 
void gameTask(void *pvParameters)
{
    esp_task_wdt_add(NULL);
    
    gameInstance.init();

    while(1)
    {
        gameInstance.run(millis());

        esp_task_wdt_reset();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/// @brief Task for running Web-Server 
void webTask(void *pvParameters)
{
    static GameState lastGameState = IDLE;
    static uint8_t lastScoreP1 = 0;
    static uint8_t lastScoreP2 = 0;

    initWebInterface();
    while(1)
    {
        GameState currentState = gameInstance.getGameState();
        GameMode currentMode = gameInstance.getGameMode();
        uint8_t scoreP1 = gameInstance.getScoreP1();
        uint8_t scoreP2 = gameInstance.getScoreP2();
        
        cleanupWebInterface();

        // --- 1. Check for status-change (Start / End) ---
        if (currentState != lastGameState) {
            if (currentState == PLAYING && lastGameState != RIPPLE_ANIM) {
                notifyGameStart(currentMode == SINGLE_PLAYER ? "single" : "multi");
                lastScoreP1 = 0;
                lastScoreP2 = 0;
            } else if (currentState == GAME_OVER || currentState == COOLDOWN) {
                notifyGameOver();
            }
            lastGameState = currentState;
        }

        // --- 2. Check for score updates ---
        if (currentState == PLAYING) {
            if (scoreP1 != lastScoreP1 || scoreP2 != lastScoreP2) {
                updateScore(scoreP1, scoreP2);
                lastScoreP1 = scoreP1;
                lastScoreP2 = scoreP2;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

/// @brief Task for updating the LEDs
void ledTask(void *pvParameters) 
{
    esp_task_wdt_add(NULL);
    leds.init();
    while (1)
    {
        leds.show();

        esp_task_wdt_reset();
        vTaskDelay(pdMS_TO_TICKS(16));
    }
}

/// @brief Task for updating the Heartbeat-LED 
void heartbeatTask(void *pvParameters)
{
    while(1)
    {
        heartbeat.update();
        vTaskDelay(pdMS_TO_TICKS(30));
    }
}

void loop() 
{
    // This function is no longer used.
}