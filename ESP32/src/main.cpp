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

// --- Variablen um Änderungen zu erkennen ---
GameState lastGameState = IDLE;
uint8_t lastScoreP1 = 0;
uint8_t lastScoreP2 = 0;

extern uint8_t scoreP1;
extern uint8_t scoreP2;
extern GameState gameState;
extern GameMode currentMode;

void setup()
{
    Serial.begin(115200);

    esp_task_wdt_config_t twdt_config = {
        .timeout_ms = 3 * 1000,
        .idle_core_mask = (1 << portNUM_PROCESSORS) - 1,    
        .trigger_panic = true, // true = ESP führt bei Timeout einen Reset durch
    };
    esp_task_wdt_init(&twdt_config);

    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN, 400000);
    Wire.setTimeOut(100);

    loadHighscores();
    
    if (!expanders.init()) { heartbeat.setError(); }
    
    leds.init();
    heartbeat.init();
    
    initWebInterface(); // <--- NEU: Webserver starten
}

void loop()
{
    if(!expanders.read(isPressed, NUM_FIELDS)) {
        heartbeat.setError();
    }
    
    runGameLogic(millis());
    
    // --- 1. Prüfen auf Status-Änderungen (Start / Ende) ---
    if (gameState != lastGameState) {
        if (gameState == PLAYING) {
            // Spiel hat gerade begonnen!
            if (currentMode == SINGLE_PLAYER) {
                notifyGameStart("single");
            } else {
                notifyGameStart("multi");
            }
            lastScoreP1 = 0;
            lastScoreP2 = 0;
        } 
        else if (gameState == GAME_OVER) {
            // Spiel ist gerade vorbei!
            notifyGameOver();
        }
        lastGameState = gameState;
    }
    
    // --- 2. Prüfen auf Score-Updates während dem Spielen ---
    if (gameState == PLAYING) {
        if (scoreP1 != lastScoreP1 || scoreP2 != lastScoreP2) {
            updateScore(scoreP1, scoreP2);
            lastScoreP1 = scoreP1;
            lastScoreP2 = scoreP2;
        }
    }
    
    leds.show();
    heartbeat.update();
    esp_task_wdt_reset();
}