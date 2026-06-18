#include "WebLog.h"
#include <ArduinoJson.h>

WebLogClass WebLog;

WebLogClass::WebLogClass() : _ws(nullptr) {
    logBuffer.reserve(MAX_LOG_LINES);
}

void WebLogClass::setWebSocket(AsyncWebSocket* ws) {
    std::lock_guard<std::recursive_mutex> lock(mtx);
    _ws = ws;
}

size_t WebLogClass::write(uint8_t c) {
    std::lock_guard<std::recursive_mutex> lock(mtx);
    Serial.write(c);
    if (currentLine.length() == 0) {
        uint32_t ms = millis();
        char timeBuf[16];
        snprintf(timeBuf, sizeof(timeBuf), "[%6lu.%03lu] ", (unsigned long)(ms / 1000), (unsigned long)(ms % 1000));
        currentLine += timeBuf;
    }
    currentLine += (char)c;
    if (c == '\n') {
        addLineToBuffer(currentLine);
        broadcastLog(currentLine);
        currentLine = "";
    }
    return 1;
}

size_t WebLogClass::write(const uint8_t *buffer, size_t size) {
    std::lock_guard<std::recursive_mutex> lock(mtx);
    Serial.write(buffer, size);
    for (size_t i = 0; i < size; i++) {
        if (currentLine.length() == 0) {
            uint32_t ms = millis();
            char timeBuf[16];
            snprintf(timeBuf, sizeof(timeBuf), "[%6lu.%03lu] ", (unsigned long)(ms / 1000), (unsigned long)(ms % 1000));
            currentLine += timeBuf;
        }
        currentLine += (char)buffer[i];
        if (buffer[i] == '\n') {
            addLineToBuffer(currentLine);
            broadcastLog(currentLine);
            currentLine = "";
        }
    }
    return size;
}

void WebLogClass::addLineToBuffer(const String& line) {
    if (logBuffer.size() >= MAX_LOG_LINES) {
        logBuffer.erase(logBuffer.begin());
    }
    logBuffer.push_back(line);
}

void WebLogClass::broadcastLog(const String& msg) {
    if (_ws != nullptr && _ws->count() > 0) {
        JsonDocument doc;
        doc["action"] = "log";
        
        String cleanMsg = msg;
        cleanMsg.trim(); // Remove newline, frontend handles newlines better in blocks
        if (cleanMsg.length() > 0) {
            doc["message"] = cleanMsg;
            String output;
            serializeJson(doc, output);
            _ws->textAll(output);
        }
    }
}

String WebLogClass::getLogsJson() {
    std::lock_guard<std::recursive_mutex> lock(mtx);
    JsonDocument doc;
    doc["action"] = "log_history";
    JsonArray arr = doc["logs"].to<JsonArray>();
    
    for (const String& line : logBuffer) {
        String cleanMsg = line;
        cleanMsg.trim();
        if (cleanMsg.length() > 0) {
            arr.add(cleanMsg);
        }
    }
    
    String output;
    serializeJson(doc, output);
    return output;
}
