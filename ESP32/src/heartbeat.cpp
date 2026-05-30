#include "heartbeat.h"

void HeartbeatLED::init()
{
    controller = &FastLED.addLeds<WS2812B, HEARTBEAT_LED_PIN, GRB>(&led, 1);
    led = CRGB::Black;
    controller->showLeds(0xFF);
}

void HeartbeatLED::update()
{
    uint8_t brightness = beatsin8(30); // 30 BPM ≈ 2 Sekunden Periode

    if (hasError)
    {
        led = CRGB(brightness, 0, 0); // Rot pulsieren
    }
    else
    {
        led = CRGB(0, brightness, 0); // Grün pulsieren
    }
    controller->showLeds(0xFF);
}

void HeartbeatLED::setError()
{
    hasError = true;
}