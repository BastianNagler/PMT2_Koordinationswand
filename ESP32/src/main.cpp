#include <Arduino.h>
#include <Wire.h>
#include <esp_timer.h>
#include <esp_task_wdt.h>

#include "GameLogic.h"
#include "config.h"
#include "leds.h"
#include "ioExpander.h"
#include "heartbeat.h"
#include "WebInterface.h"


volatile bool isPressed[NUM_FIELDS];
LED_Driver leds;
HeartbeatLED heartbeat;
GameLogic gameInstance(isPressed, leds);


void inputTask(void *pvParameters);
void gameTask(void *pvParameters);
void webTask(void *pvParameters);
void ledTask(void *pvParameters);
void heartbeatTask(void *pvParameters);

void setup()
{
    Serial.begin(115200);

    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN, 400000);
    Wire.setTimeOut(100);

    xTaskCreate(inputTask, "Input Task", 4096, NULL, 4, NULL);
    xTaskCreate(gameTask, "Game Task", 4096, NULL, 3, NULL);
    xTaskCreate(ledTask, "LED Task", 4096, NULL, 2, NULL);
    xTaskCreate(webTask, "Web Task", 32768, NULL, 1, NULL);
    xTaskCreate(heartbeatTask, "Heartbeat Task", 4096, NULL, 0, NULL);
    vTaskDelete(NULL);
}

/// @brief Task for handling hardware inputs. High priority to ensure responsiveness.
void inputTask(void *pvParameters) 
{
    esp_task_wdt_add(NULL);

    static const uint8_t io_exp_irq_pins[NUM_IO_EXPANDER] = IO_EXPANDER_IRQ_PINS;
    static const uint8_t io_exp_addresses[NUM_IO_EXPANDER] = IO_EXPANDER_ADDRESSES;
    static IO_Expander expanders(NUM_IO_EXPANDER, io_exp_addresses, io_exp_irq_pins);

    if (!expanders.init())
    {
        heartbeat.setError();
    }

    while (1) {
        if (!expanders.read(isPressed, NUM_FIELDS))
        {
            heartbeat.setError();
        }

        esp_task_wdt_reset();
        vTaskDelay(pdMS_TO_TICKS(20));
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
        
        // --- 1. Check for status-change (Start / End) ---
        if (currentState != lastGameState) {
            if (currentState == PLAYING) {
                notifyGameStart(currentMode == SINGLE_PLAYER ? "single" : "multi");
                lastScoreP1 = 0;
                lastScoreP2 = 0;
            } else if (currentState == GAME_OVER) {
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
    heartbeat.init();
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