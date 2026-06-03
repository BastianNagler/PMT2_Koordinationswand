#include "heartbeat.h"

void HeartbeatLED::update()
{
    uint8_t brightness = beatsin8(30); // 30 BPM ≈ 2 Sekunden Periode

    if (hasError)
    {
        color = (brightness << 16); // Rot pulsieren
    }
    else
    {
        color = (brightness << 8); // Grün pulsieren
    }
    ws2812Write(HEARTBEAT_LED_PIN, color);
}

void HeartbeatLED::setError()
{
    hasError = true;
}