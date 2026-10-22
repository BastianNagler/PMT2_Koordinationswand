#include "heartbeat.h"
#include "WebLog.h"

void HeartbeatLED::update() {
    // 30 BPM sine wave generates an approximate 2-second organic breathing period
    const uint8_t brightness = beatsin8(30);

    if (hasError.load()) {
        color = static_cast<uint32_t>(brightness) << 8;   // Pulsing Red (GRB format)
    } else {
        color = static_cast<uint32_t>(brightness) << 16;  // Pulsing Green (GRB format)
    }

    ws2812Write(HEARTBEAT_LED_PIN, color);
}

void HeartbeatLED::setError() {
    WebLog.println("[HEARTBEAT] System error flagged");
    hasError.store(true);
}
