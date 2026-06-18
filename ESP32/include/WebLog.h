#ifndef WEBLOG_H
#define WEBLOG_H

#include <Arduino.h>
#include <vector>
#include <ESPAsyncWebServer.h>
#include <mutex>

#define MAX_LOG_LINES 50

class WebLogClass : public Print {
private:
    std::vector<String> logBuffer;
    String currentLine;
    AsyncWebSocket* _ws;
    std::mutex mtx;

    void broadcastLog(const String& msg);
    void addLineToBuffer(const String& line);

public:
    WebLogClass();
    virtual size_t write(uint8_t);
    virtual size_t write(const uint8_t *buffer, size_t size);

    // Get all buffered logs as a JSON object string
    String getLogsJson();

    // Set the websocket for broadcasting logs
    void setWebSocket(AsyncWebSocket* ws);
};

extern WebLogClass WebLog;

#endif
