#include "heartbeat.h"

void HeartbeatLED::init()
{
    FastLED.addLeds<WS2812B, HEARTBEAT_LED_PIN, GRB>(&led, 1);
    led = CRGB::Black;
    FastLED.show();
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
    FastLED.show();
}

void HeartbeatLED::setError()
{
    hasError = true;
}