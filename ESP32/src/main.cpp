#include <Arduino.h>
#include <Wire.h>
#include <esp_timer.h>
#include <esp_task_wdt.h>
#include <LittleFS.h>
#include <cstdint>

#include "GameLogic.h"
#include "SensorState.h"
#include "config.h"
#include "leds.h"
#include "ioExpander.h"
#include "heartbeat.h"
#include "WebInterface.h"
#include "WebLog.h"

/** Global lock-free atomic sensor state container shared across tasks. */
SensorState sensorState;

/** Hardware LED driver controlling parallel WS2812B channels. */
LED_Driver leds;

/** Heartbeat controller managing the visual status LED. */
HeartbeatLED heartbeat;

/** Core game state machine engine. */
GameLogic gameInstance(sensorState, leds);

static void inputTask(void* pvParameters);
static void gameTask(void* pvParameters);
static void webTask(void* pvParameters);
static void ledTask(void* pvParameters);
static void heartbeatTask(void* pvParameters);

/**
 * Panic handler invoked on unrecoverable CPU fault to illuminate red status indication.
 *
 * @param[in] info System panic metadata structure.
 * @param[in] arg Optional context argument.
 */
static void custom_panic_handler(arduino_panic_info_t* const info, void* const arg) {
    ws2812Write(HEARTBEAT_LED_PIN, 0xFF0000);
}

void setup() {
    delay(500);
    Serial.setTxBufferSize(1024);
    Serial.begin(115200);

    WebLog.println("[SETUP] Booting cWall ESP32-S3...");

    if (!LittleFS.begin(true)) {
        WebLog.println("[ERROR] LittleFS mounting failed at boot");
    } else {
        WebLog.println("[SETUP] LittleFS mounted successfully");
    }

    recoverI2CBus(I2C_SDA_PIN, I2C_SCL_PIN);

    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN, I2C_FREQ_HZ);
    Wire.setTimeOut(100);

    set_arduino_panic_handler(custom_panic_handler, nullptr);

    BaseType_t ret;

    ret = xTaskCreatePinnedToCore(inputTask, "Input Task", 4096, nullptr, 4, nullptr, 1);
    if (ret != pdPASS) {
        WebLog.printf("[ERROR] Failed to create Input Task (err: %d)\n", ret);
    }

    ret = xTaskCreatePinnedToCore(gameTask, "Game Task", 4096, nullptr, 3, nullptr, 1);
    if (ret != pdPASS) {
        WebLog.printf("[ERROR] Failed to create Game Task (err: %d)\n", ret);
    }

    ret = xTaskCreatePinnedToCore(ledTask, "LED Task", 4096, nullptr, 2, nullptr, 1);
    if (ret != pdPASS) {
        WebLog.printf("[ERROR] Failed to create LED Task (err: %d)\n", ret);
    }

    ret = xTaskCreatePinnedToCore(heartbeatTask, "Heartbeat Task", 2048, nullptr, 1, nullptr, 1);
    if (ret != pdPASS) {
        WebLog.printf("[ERROR] Failed to create Heartbeat Task (err: %d)\n", ret);
    }

    ret = xTaskCreatePinnedToCore(webTask, "Web Task", 8192, nullptr, 1, nullptr, 0);
    if (ret != pdPASS) {
        WebLog.printf("[ERROR] Failed to create Web Task (err: %d)\n", ret);
    }

    WebLog.println("[SETUP] Initialization completed, all tasks dispatched");
}

/**
 * High-priority input polling task scanning I/O expanders and discrete inputs.
 */
static void inputTask(void* pvParameters) {
    WebLog.println("[TASK] Input Task running on Core 1");
    static IO_Expander expanders(NUM_IO_EXPANDER, IO_EXPANDER_ADDRESSES, IO_EXPANDER_IRQ_PINS);

    if (!expanders.init()) {
        heartbeat.setError();
    }

    // Hardware errata workaround: PCB trace for sensor box 16 routed directly to discrete GPIO
    pinMode(SENSOR_16_OVERRIDE_PIN, INPUT);
    pinMode(ONBOARD_BOOT_BUTTON_PIN, INPUT_PULLUP);

    const esp_err_t wdtErr = esp_task_wdt_add(nullptr);
    if (wdtErr != ESP_OK) {
        WebLog.printf("[WDT] [INFO] Input Task TWDT not active (0x%X)\n", wdtErr);
    }

    while (true) {
        if (!expanders.read(sensorState, NUM_FIELDS)) {
            WebLog.println("[ERROR] IO-Expander read fault, initiating bus recovery");
            heartbeat.setError();
            if (expanders.resetAndReinit()) {
                WebLog.println("[IO-EXPANDER] Expanders reset and re-initialized");
            } else {
                WebLog.println("[ERROR] Failed to restore expanders after reset");
            }
        }

        // Apply hardware errata reading for sensor 16
        if (NUM_FIELDS >= 17) {
            sensorState.setPressed(16, digitalRead(SENSOR_16_OVERRIDE_PIN) == HIGH);
        }

        // Mirror onboard BOOT push-button to sensor box 0 for hardware-less desk testing
        const bool bootPressed = (digitalRead(ONBOARD_BOOT_BUTTON_PIN) == LOW);
        if (bootPressed) {
            sensorState.setPressed(0, true);
        } else if (heartbeat.getError()) {
            sensorState.setPressed(0, false);
        }

        esp_task_wdt_reset();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/**
 * Core game task calculating transitions and animations at 100 Hz.
 */
static void gameTask(void* pvParameters) {
    WebLog.println("[TASK] Game Task running on Core 1");
    const esp_err_t wdtErr = esp_task_wdt_add(nullptr);
    if (wdtErr != ESP_OK) {
        WebLog.printf("[WDT] [INFO] Game Task TWDT not active (0x%X)\n", wdtErr);
    }

    gameInstance.init();

    while (true) {
        gameInstance.run(millis());

        esp_task_wdt_reset();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/**
 * Networking task hosting the SoftAP web server and WebSocket broadcaster.
 */
static void webTask(void* pvParameters) {
    WebLog.println("[TASK] Web Task running on Core 0");
    const esp_err_t wdtErr = esp_task_wdt_add(nullptr);
    if (wdtErr != ESP_OK) {
        WebLog.printf("[WDT] [INFO] Web Task TWDT not active (0x%X)\n", wdtErr);
    }

    static GameState lastGameState = IDLE;
    static uint8_t lastScoreP1 = 0;
    static uint8_t lastScoreP2 = 0;

    initWebInterface();

    while (true) {
        const GameState currentState = gameInstance.getGameState();
        const GameMode currentMode = gameInstance.getGameMode();
        const uint8_t scoreP1 = gameInstance.getScoreP1();
        const uint8_t scoreP2 = gameInstance.getScoreP2();

        cleanupWebInterface();

        if (currentState != lastGameState) {
            if (currentState == PLAYING && lastGameState != RIPPLE_ANIM) {
                notifyGameStart((currentMode == SINGLE_PLAYER) ? "single" : "multi");
                lastScoreP1 = 0;
                lastScoreP2 = 0;
            } else if (currentState == GAME_OVER || currentState == COOLDOWN) {
                notifyGameOver();
            }
            lastGameState = currentState;
        }

        if (currentState == PLAYING) {
            if (scoreP1 != lastScoreP1 || scoreP2 != lastScoreP2) {
                updateScore(scoreP1, scoreP2);
                lastScoreP1 = scoreP1;
                lastScoreP2 = scoreP2;
            }
        }

        esp_task_wdt_reset();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

/**
 * High-priority display task rendering the frame buffer to physical WS2812 strips.
 */
static void ledTask(void* pvParameters) {
    WebLog.println("[TASK] LED Task running on Core 1");
    const esp_err_t wdtErr = esp_task_wdt_add(nullptr);
    if (wdtErr != ESP_OK) {
        WebLog.printf("[WDT] [INFO] LED Task TWDT not active (0x%X)\n", wdtErr);
    }

    leds.init();

    while (true) {
        leds.show();

        esp_task_wdt_reset();
        vTaskDelay(pdMS_TO_TICKS(16));
    }
}

/**
 * Low-priority task driving the heartbeat status LED.
 */
static void heartbeatTask(void* pvParameters) {
    WebLog.println("[TASK] Heartbeat Task running on Core 1");
    while (true) {
        heartbeat.update();
        vTaskDelay(pdMS_TO_TICKS(30));
    }
}

void loop() {
    vTaskDelay(pdMS_TO_TICKS(5000));
    WebLog.printf("[SYSTEM] Free Heap: %u bytes\n", ESP.getFreeHeap());
}
