#pragma once

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <cstdint>
#include <cstddef>
#include <mutex>
#include <array>

/** Maximum number of log lines retained in the ring buffer. */
inline constexpr size_t MAX_LOG_LINES = 50;

/** Maximum length of a single log message including timestamp and null terminator. */
inline constexpr size_t MAX_LOG_LINE_LEN = 128;

/**
 * Thread-safe, statically allocated circular log buffer.
 *
 * Real-time tasks (`inputTask`, `gameTask`, `ledTask`) write to this buffer
 * with strictly zero dynamic heap allocations. WebSocket distribution is decoupled
 * and performed solely inside the lower-priority `webTask`.
 */
class WebLogClass : public Print {
private:
    char ringBuffer[MAX_LOG_LINES][MAX_LOG_LINE_LEN];
    size_t ringHead = 0;
    size_t ringCount = 0;
    size_t totalLogged = 0;
    size_t lastBroadcastSeq = 0;

    char currentLine[MAX_LOG_LINE_LEN];
    size_t currentLineLen = 0;

    AsyncWebSocket* webSocket = nullptr;
    mutable std::mutex logMutex;

    void commitCurrentLine();

public:
    WebLogClass();
    ~WebLogClass() override = default;
    WebLogClass(const WebLogClass&) = delete;
    WebLogClass& operator=(const WebLogClass&) = delete;

    /**
     * Writes a single character to Serial and appends to the internal line buffer.
     *
     * @param[in] c Character byte to write.
     *
     * @return Number of bytes processed (1).
     */
    size_t write(const uint8_t c) override;

    /**
     * Writes an array of character bytes to Serial and internal line buffer.
     *
     * @param[in] buffer Pointer to data bytes.
     * @param[in] size Number of bytes to write.
     *
     * @return Total number of bytes written.
     */
    size_t write(const uint8_t* const buffer, const size_t size) override;

    /**
     * Associates the pointer to the active WebSocket server instance.
     *
     * @param[in] ws Pointer to the server `AsyncWebSocket` instance.
     */
    void setWebSocket(AsyncWebSocket* const ws);

    /**
     * Periodically called strictly within `webTask` to flush newly arrived log
     * messages to connected WebSocket clients without blocking real-time loops.
     */
    void flushPendingLogs();

    /**
     * Exports all currently held log lines into an ArduinoJson document.
     *
     * @note Must only be invoked from within `webTask` context.
     *
     * @return Serialized JSON string containing full log history array.
     */
    String getLogsJson();
};

extern WebLogClass WebLog;
