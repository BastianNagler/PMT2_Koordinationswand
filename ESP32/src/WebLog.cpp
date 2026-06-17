#include "WebLog.h"
#include <ArduinoJson.h>

WebLogClass WebLog;

WebLogClass::WebLogClass() : _ws(nullptr) {
    logBuffer.reserve(MAX_LOG_LINES);
}

void WebLogClass::setWebSocket(AsyncWebSocket* ws) {
    _ws = ws;
}

size_t WebLogClass::write(uint8_t c) {
    Serial.write(c);
    currentLine += (char)c;
    if (c == '\n') {
        addLineToBuffer(currentLine);
        broadcastLog(currentLine);
        currentLine = "";
    }
    return 1;
}

size_t WebLogClass::write(const uint8_t *buffer, size_t size) {
    Serial.write(buffer, size);
    for (size_t i = 0; i < size; i++) {
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
