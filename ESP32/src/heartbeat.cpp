#include "heartbeat.h"
#include <cmath> // Für sin()

void HeartbeatLED::init()
{
    FastLED.addLeds<WS2812B, HEARTBEAT_LED_PIN, GRB>(&led, 1);
    led = CRGB::Black;
    FastLED.show();
}

void HeartbeatLED::update()
{
    uint32_t currentTime = millis();
    // Sinusverlauf: Periode ca. 2 Sekunden (2*PI / 0.00314 ≈ 2000ms)
    float angle = currentTime * 0.00314f; // Skaliere für Periode
    uint8_t brightness = (sin(angle) + 1.0f) * 127.5f; // 0-255

    if (hasError)
    {
        led = CRGB(brightness, 0, 0); // Rot pulsieren
    }
    else
    {
        led = CRGB(0, brightness, 0); // Grün pulsieren
    }
    FastLED.show();
}

void HeartbeatLED::setError()
{
    hasError = true;
}