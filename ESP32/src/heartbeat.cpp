#include "heartbeat.h"
#include "WebLog.h"

void HeartbeatLED::update() {
    if (ONBOARD_LED_MIRRORS_BOX_0) {
        return;
    }

    const uint8_t brightness = beatsin8(30);
    const uint32_t color = static_cast<uint32_t>(brightness) << (hasError.load() ? 16 : 8);
    ws2812Write(HEARTBEAT_LED_PIN, color);
}

void HeartbeatLED::setError() {
    WebLog.println("[HEARTBEAT] System error flagged");
    hasError.store(true);
}
