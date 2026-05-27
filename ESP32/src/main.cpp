#include <Arduino.h>
#include <Wire.h>
#include <esp_timer.h>
#include <esp_task_wdt.h>

#include "TwallGame.h"
#include "config.h"
#include "leds.h"
#include "ioExpander.h"
#include "heartbeat.h"
#include "WebInterface.h"

const uint8_t io_exp_irq_pins[NUM_IO_EXPANDER] = IO_EXPANDER_IRQ_PINS;
const uint8_t io_exp_addresses[NUM_IO_EXPANDER] = IO_EXPANDER_ADDRESSES;

IO_Expander expanders(NUM_IO_EXPANDER, io_exp_addresses, io_exp_irq_pins);
volatile bool isPressed[NUM_FIELDS];
LED_Driver leds;
HeartbeatLED heartbeat;

GameState lastGameState = IDLE;
uint8_t lastScoreP1 = 0;
uint8_t lastScoreP2 = 0;

extern uint8_t scoreP1;
extern uint8_t scoreP2;
extern GameState gameState;
extern GameMode currentMode;

void inputTask(void *pvParameters);
void gameAndWebTask(void *pvParameters);
void ledAndHeartbeatTask(void *pvParameters);

void setup()
{
    Serial.begin(115200);

    esp_task_wdt_config_t twdt_config = {
        .timeout_ms = 3 * 1000,
        .idle_core_mask = (1 << portNUM_PROCESSORS) - 1,    
        .trigger_panic = true,
    };
    esp_task_wdt_init(&twdt_config);

    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN, 400000);
    Wire.setTimeOut(100);

    loadHighscores();
    
    if (!expanders.init()) { heartbeat.setError(); }
    
    leds.init();
    heartbeat.init();
    
    initWebInterface();

    xTaskCreate(inputTask, "Input Task", 2048, NULL, 5, NULL);
    xTaskCreate(gameAndWebTask, "Game/Web Task", 4096, NULL, 3, NULL);
    xTaskCreate(ledAndHeartbeatTask, "LED Task", 2048, NULL, 2, NULL);
    vTaskDelete(NULL);
}

/**
 * @brief Task for handling hardware inputs. High priority to ensure responsiveness.
 */
void inputTask(void *pvParameters) 
{
    while (1) {
        if (!expanders.read(isPressed, NUM_FIELDS)) {
            heartbeat.setError();
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }

}

/**
 * @brief Task for running the main game logic and sending web updates.
 */
void gameAndWebTask(void *pvParameters) 
{
    while (1) {
        runGameLogic(millis());

        // --- 1. Check for status-change (Start / End) ---
        if (gameState != lastGameState) {
            if (gameState == PLAYING) {
                notifyGameStart(currentMode == SINGLE_PLAYER ? "single" : "multi");
                lastScoreP1 = 0;
                lastScoreP2 = 0;
            } else if (gameState == GAME_OVER) {
                notifyGameOver();
            }
            lastGameState = gameState;
        }

        // --- 2. Check for score updates ---
        if (gameState == PLAYING) {
            if (scoreP1 != lastScoreP1 || scoreP2 != lastScoreP2) {
                updateScore(scoreP1, scoreP2);
                lastScoreP1 = scoreP1;
                lastScoreP2 = scoreP2;
            }
        }

        esp_task_wdt_reset(); // Reset watchdog in the main logic task
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/**
 * @brief Task for updating the LED strip and heartbeat. Low priority.
 */
void ledAndHeartbeatTask(void *pvParameters) 
{
    while (1) {
        leds.show();
        heartbeat.update();
        vTaskDelay(pdMS_TO_TICKS(16));
    }
}

void loop() 
{
    // This function is no longer used.
}