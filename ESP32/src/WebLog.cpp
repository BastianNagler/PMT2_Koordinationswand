#include "WebLog.h"
#include <ArduinoJson.h>
#include <cstdio>
#include <cstring>

WebLogClass WebLog;

WebLogClass::WebLogClass() {
    memset(ringBuffer, 0, sizeof(ringBuffer));
    memset(currentLine, 0, sizeof(currentLine));
}

void WebLogClass::setWebSocket(AsyncWebSocket* const ws) {
    std::lock_guard<std::mutex> lock(logMutex);
    webSocket = ws;
}

void WebLogClass::commitCurrentLine() {
    if (currentLineLen == 0) {
        return;
    }

    currentLine[currentLineLen] = '\0';
    memcpy(ringBuffer[ringHead], currentLine, currentLineLen + 1);

    ringHead = (ringHead + 1) % MAX_LOG_LINES;
    if (ringCount < MAX_LOG_LINES) {
        ringCount++;
    }
    totalLogged++;
    currentLineLen = 0;
}

size_t WebLogClass::write(const uint8_t c) {
    std::lock_guard<std::mutex> lock(logMutex);
    Serial.write(c);

    if (currentLineLen == 0 && c != '\r' && c != '\n') {
        const uint32_t ms = millis();
        const int written = snprintf(currentLine, sizeof(currentLine),
                                     "[%6lu.%03lu] ",
                                     static_cast<unsigned long>(ms / 1000),
                                     static_cast<unsigned long>(ms % 1000));
        if (written > 0 && static_cast<size_t>(written) < sizeof(currentLine)) {
            currentLineLen = static_cast<size_t>(written);
        }
    }

    if (c == '\n') {
        commitCurrentLine();
    } else if (c != '\r') {
        if (currentLineLen + 1 < sizeof(currentLine)) {
            currentLine[currentLineLen++] = static_cast<char>(c);
        } else {
            commitCurrentLine();
        }
    }

    return 1;
}

size_t WebLogClass::write(const uint8_t* const buffer, const size_t size) {
    if (buffer == nullptr || size == 0) {
        return 0;
    }

    std::lock_guard<std::mutex> lock(logMutex);
    Serial.write(buffer, size);

    for (size_t i = 0; i < size; ++i) {
        const uint8_t c = buffer[i];

        if (currentLineLen == 0 && c != '\r' && c != '\n') {
            const uint32_t ms = millis();
            const int written = snprintf(currentLine, sizeof(currentLine),
                                         "[%6lu.%03lu] ",
                                         static_cast<unsigned long>(ms / 1000),
                                         static_cast<unsigned long>(ms % 1000));
            if (written > 0 && static_cast<size_t>(written) < sizeof(currentLine)) {
                currentLineLen = static_cast<size_t>(written);
            }
        }

        if (c == '\n') {
            commitCurrentLine();
        } else if (c != '\r') {
            if (currentLineLen + 1 < sizeof(currentLine)) {
                currentLine[currentLineLen++] = static_cast<char>(c);
            } else {
                commitCurrentLine();
            }
        }
    }

    return size;
}

void WebLogClass::flushPendingLogs() {
    AsyncWebSocket* wsTarget = nullptr;
    char linesToSend[8][MAX_LOG_LINE_LEN];
    size_t sendCount = 0;

    {
        std::lock_guard<std::mutex> lock(logMutex);
        if (webSocket == nullptr || webSocket->count() == 0 || lastBroadcastSeq >= totalLogged) {
            return;
        }

        wsTarget = webSocket;
        const size_t pending = totalLogged - lastBroadcastSeq;
        sendCount = (pending > 8) ? 8 : pending;

        // Determine starting slot in ring buffer
        for (size_t i = 0; i < sendCount; ++i) {
            const size_t seq = lastBroadcastSeq + i;
            const size_t offsetFromCurrent = totalLogged - 1 - seq;
            if (offsetFromCurrent < ringCount) {
                const size_t idx = (ringHead + MAX_LOG_LINES - 1 - offsetFromCurrent) % MAX_LOG_LINES;
                memcpy(linesToSend[i], ringBuffer[idx], MAX_LOG_LINE_LEN);
            } else {
                linesToSend[i][0] = '\0';
            }
        }
        lastBroadcastSeq += sendCount;
    }

    for (size_t i = 0; i < sendCount; ++i) {
        if (linesToSend[i][0] != '\0') {
            JsonDocument doc;
            doc["action"] = "log";
            doc["message"] = linesToSend[i];
            String output;
            serializeJson(doc, output);
            wsTarget->textAll(output);
        }
    }
}

String WebLogClass::getLogsJson() {
    std::lock_guard<std::mutex> lock(logMutex);
    JsonDocument doc;
    doc["action"] = "log_history";
    JsonArray arr = doc["logs"].to<JsonArray>();

    const size_t startIdx = (ringCount < MAX_LOG_LINES)
                                ? 0
                                : ringHead;

    for (size_t i = 0; i < ringCount; ++i) {
        const size_t idx = (startIdx + i) % MAX_LOG_LINES;
        if (ringBuffer[idx][0] != '\0') {
            arr.add(ringBuffer[idx]);
        }
    }

    String output;
    serializeJson(doc, output);
    return output;
}
