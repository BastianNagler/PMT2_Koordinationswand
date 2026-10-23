#include "WebInterface.h"
#include <WiFi.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>

#include "GameLogic.h"
#include "WebLog.h"

extern GameLogic gameInstance;

static AsyncWebServer server(80);
static AsyncWebSocket ws("/ws");
static std::mutex wsBroadcastMutex;
static DNSServer dnsServer;

static constexpr const char* SSID_NAME = "C-Wall";
static constexpr const char* SSID_PASS = "cwallpassword";

static const IPAddress localIP(192, 168, 4, 1);
static const IPAddress localGW(192, 168, 4, 1);
static const IPAddress localSN(255, 255, 255, 0);

inline constexpr size_t MAX_WS_CLIENTS = 6;
inline constexpr uint32_t CLIENT_INACTIVITY_TIMEOUT_MS = 25000;
inline constexpr size_t MAX_CLIENT_QUEUE_LEN = 2;
inline constexpr uint32_t MIN_SAFE_HEAP_BYTES = 45000;
inline constexpr uint32_t CRITICAL_HEAP_BYTES = 30000;
inline constexpr uint8_t MAX_SOFTAP_CONNECTIONS = 8;

struct ClientMeta {
    uint32_t id = 0;
    uint32_t lastSeenMs = 0;
    bool isLogSubscriber = false;
};

static std::array<ClientMeta, MAX_WS_CLIENTS> activeClients{};
static std::mutex clientMetaMutex;

static ClientMeta* getClientMetaUnsafe(const uint32_t clientId) {
    for (auto& entry : activeClients) {
        if (entry.id == clientId) {
            return &entry;
        }
    }
    return nullptr;
}

static void registerClient(const uint32_t clientId) {
    std::lock_guard<std::mutex> lock(clientMetaMutex);
    for (auto& entry : activeClients) {
        if (entry.id == clientId) {
            entry.lastSeenMs = millis();
            entry.isLogSubscriber = false;
            return;
        }
    }
    for (auto& entry : activeClients) {
        if (entry.id == 0) {
            entry.id = clientId;
            entry.lastSeenMs = millis();
            entry.isLogSubscriber = false;
            return;
        }
    }
}

static void unregisterClient(const uint32_t clientId) {
    std::lock_guard<std::mutex> lock(clientMetaMutex);
    for (auto& entry : activeClients) {
        if (entry.id == clientId) {
            entry.id = 0;
            entry.lastSeenMs = 0;
            entry.isLogSubscriber = false;
            return;
        }
    }
}

static void markActivity(const uint32_t clientId) {
    std::lock_guard<std::mutex> lock(clientMetaMutex);
    ClientMeta* const meta = getClientMetaUnsafe(clientId);
    if (meta != nullptr) {
        meta->lastSeenMs = millis();
    }
}

static void setLogSubscription(const uint32_t clientId, const bool subscribed) {
    std::lock_guard<std::mutex> lock(clientMetaMutex);
    ClientMeta* const meta = getClientMetaUnsafe(clientId);
    if (meta != nullptr) {
        meta->isLogSubscriber = subscribed;
    }
}

static bool isLogSubscriber(const uint32_t clientId) {
    std::lock_guard<std::mutex> lock(clientMetaMutex);
    for (const auto& entry : activeClients) {
        if (entry.id == clientId && entry.isLogSubscriber) {
            return true;
        }
    }
    return false;
}

static uint32_t getClientLastSeen(const uint32_t clientId) {
    std::lock_guard<std::mutex> lock(clientMetaMutex);
    for (const auto& entry : activeClients) {
        if (entry.id == clientId) {
            return entry.lastSeenMs;
        }
    }
    return 0;
}

static void broadcastWs(const String& message) {
    if (ws.count() == 0) {
        return;
    }
    std::lock_guard<std::mutex> lock(wsBroadcastMutex);
    ws.textAll(message);
}

static String formatHexColor(const uint32_t color) {
    char buf[10];
    snprintf(buf, sizeof(buf), "#%06lX", static_cast<unsigned long>(color & 0xFFFFFF));
    return String(buf);
}

static uint32_t parseHexColor(const char* const hexStr) {
    if (hexStr == nullptr) {
        return 0;
    }
    const char* ptr = hexStr;
    if (ptr[0] == '#') {
        ptr++;
    }
    if (strlen(ptr) < 6) {
        return 0;
    }
    return static_cast<uint32_t>(strtoul(ptr, nullptr, 16));
}

static bool isHeapSafe(AsyncWebServerRequest* const request) {
    if (ESP.getFreeHeap() < MIN_SAFE_HEAP_BYTES) {
        AsyncWebServerResponse* const response =
            request->beginResponse(503, "text/plain", "503 Service Unavailable: Server Under Heavy Load");
        response->addHeader("Retry-After", "3");
        request->send(response);
        return false;
    }
    return true;
}

static void sendStateToClient(AsyncWebSocketClient* const client) {
    const GameState state = gameInstance.getGameState();
    const uint32_t duration = gameInstance.getGameDurationMs();
    const bool isDefaultTime = (duration == 60000);

    if (state == PLAYING || state == START_ANIM || state == RIPPLE_ANIM) {
        JsonDocument docStart;
        docStart["action"] = "start game";
        docStart["mode"] = (gameInstance.getGameMode() == SINGLE_PLAYER) ? "single" : "multi";

        uint32_t elapsed = 0;
        if (state == PLAYING || state == RIPPLE_ANIM) {
            elapsed = millis() - gameInstance.getGameStartTime();
        }
        const uint32_t remaining = (duration > elapsed) ? (duration - elapsed) : 0;
        docStart["durationMs"] = remaining;

        String outputStart;
        serializeJson(docStart, outputStart);
        client->text(outputStart);

        JsonDocument docScore;
        docScore["action"] = "update counter";
        docScore["scoreP1"] = gameInstance.getScoreP1();
        docScore["scoreP2"] = gameInstance.getScoreP2();
        docScore["counterValue"] = gameInstance.getScoreP1();

        String outputScore;
        serializeJson(docScore, outputScore);
        client->text(outputScore);
    } else {
        JsonDocument doc;
        doc["action"] = "end of game";
        doc["finalScoreP1"] = gameInstance.getScoreP1();
        doc["finalScoreP2"] = gameInstance.getScoreP2();
        doc["isDefaultTime"] = isDefaultTime;

        const std::array<HighscoreEntry, NUM_HIGHSCORES> highscores = gameInstance.getHighscoresCopy(isDefaultTime);
        JsonArray hsArray = doc["highscoreList"].to<JsonArray>();
        for (size_t i = 0; i < NUM_HIGHSCORES; ++i) {
            JsonObject player = hsArray.add<JsonObject>();
            player["playerName"] = highscores[i].name;
            player["counterValue"] = highscores[i].score;
        }

        String output;
        serializeJson(doc, output);
        client->text(output);
    }
}

static void onEvent(AsyncWebSocket* const serverInstance, AsyncWebSocketClient* const client,
                    const AwsEventType type, void* const arg, uint8_t* const data, const size_t len) {
    if (type == WS_EVT_CONNECT) {
        const uint32_t freeHeap = ESP.getFreeHeap();
        if (freeHeap < MIN_SAFE_HEAP_BYTES) {
            WebLog.printf("[WEBSOCKET] Rejected client %u: Low heap (%u bytes)\n", client->id(), freeHeap);
            client->close(1013, "Server busy (Low memory)");
            return;
        }

        if (ws.count() > MAX_WS_CLIENTS) {
            AsyncWebSocketClient* evictCandidate = nullptr;
            const uint32_t now = millis();

            for (AsyncWebSocketClient& c : ws.getClients()) {
                if (c.id() != client->id() && c.status() == WS_CONNECTED) {
                    if (c.queueLen() > 0) {
                        evictCandidate = &c;
                        break;
                    }
                    const uint32_t lastSeen = getClientLastSeen(c.id());
                    if (lastSeen > 0 && (now - lastSeen > 10000)) {
                        evictCandidate = &c;
                        break;
                    }
                }
            }

            if (evictCandidate != nullptr) {
                WebLog.printf("[WEBSOCKET] Evicting stale client %u for incoming client %u\n",
                              evictCandidate->id(), client->id());
                evictCandidate->close(1000, "Evicted for active user");
                unregisterClient(evictCandidate->id());
                ws.cleanupClients();
            } else {
                WebLog.printf("[WEBSOCKET] Rejected client %u: Capacity reached (%u clients)\n",
                              client->id(), ws.count());
                client->close(1013, "Maximum clients connected");
                return;
            }
        }

        client->setCloseClientOnQueueFull(true);
        client->keepAlivePeriod(8);
        registerClient(client->id());

        WebLog.printf("[WEBSOCKET] Client connected (ID: %u, IP: %s, Total: %u)\n",
                       client->id(), client->remoteIP().toString().c_str(), ws.count());
        sendStateToClient(client);

    } else if (type == WS_EVT_DISCONNECT) {
        WebLog.printf("[WEBSOCKET] Client disconnected (ID: %u)\n", client->id());
        unregisterClient(client->id());
        ws.cleanupClients();

    } else if (type == WS_EVT_ERROR) {
        WebLog.printf("[WEBSOCKET] Client error (ID: %u)\n", client->id());
        unregisterClient(client->id());

    } else if (type == WS_EVT_PONG) {
        markActivity(client->id());

    } else if (type == WS_EVT_DATA) {
        markActivity(client->id());
        const AwsFrameInfo* const info = static_cast<AwsFrameInfo*>(arg);
        if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
            JsonDocument doc;
            const DeserializationError error = deserializeJson(doc, data, len);

            if (!error) {
                const char* const action = doc["action"];
                if (action != nullptr) {
                    if (strcmp(action, "set name") == 0) {
                        const char* const newName = doc["playerName"];
                        const int index = doc["index"];
                        const bool isDefaultTime = (gameInstance.getGameDurationMs() == 60000);
                        gameInstance.updateHighscoreName(index, newName, isDefaultTime);
                        notifyGameOver();
                    } else if (strcmp(action, "get_logs") == 0) {
                        setLogSubscription(client->id(), true);
                        const String logsJson = WebLog.getLogsJson();
                        client->text(logsJson);
                    } else if (strcmp(action, "get_config") == 0) {
                        const GameSettings currentSettings = gameInstance.getSettings();
                        JsonDocument configDoc;
                        configDoc["action"] = "config_data";
                        configDoc["gameDurationMs"] = currentSettings.gameDurationMs;
                        configDoc["colorStartAnim"] = formatHexColor(currentSettings.colorStartAnim);
                        configDoc["colorP1"] = formatHexColor(currentSettings.colorP1);
                        configDoc["colorP2"] = formatHexColor(currentSettings.colorP2);
                        configDoc["colorP1Ripple"] = formatHexColor(currentSettings.colorP1Ripple);
                        configDoc["colorP2Ripple"] = formatHexColor(currentSettings.colorP2Ripple);

                        String output;
                        serializeJson(configDoc, output);
                        client->text(output);
                    } else if (strcmp(action, "set_config") == 0) {
                        if (!doc["gameDurationMs"].isNull()) {
                            gameInstance.applyNewDuration(doc["gameDurationMs"]);
                        }
                        GameSettings newSettings = gameInstance.getSettings();
                        if (!doc["colorStartAnim"].isNull()) {
                            newSettings.colorStartAnim = parseHexColor(doc["colorStartAnim"]);
                        }
                        if (!doc["colorP1"].isNull()) {
                            newSettings.colorP1 = parseHexColor(doc["colorP1"]);
                        }
                        if (!doc["colorP2"].isNull()) {
                            newSettings.colorP2 = parseHexColor(doc["colorP2"]);
                        }
                        if (!doc["colorP1Ripple"].isNull()) {
                            newSettings.colorP1Ripple = parseHexColor(doc["colorP1Ripple"]);
                        }
                        if (!doc["colorP2Ripple"].isNull()) {
                            newSettings.colorP2Ripple = parseHexColor(doc["colorP2Ripple"]);
                        }
                        gameInstance.setColors(newSettings.colorStartAnim, newSettings.colorP1,
                                               newSettings.colorP2, newSettings.colorP1Ripple,
                                               newSettings.colorP2Ripple);
                        WebLog.println("[WEB] Applied new configuration settings");
                    } else if (strcmp(action, "reset_highscore") == 0) {
                        gameInstance.reset60sHighscore();
                        notifyGameOver();
                    }
                }
            } else {
                WebLog.println("[ERROR] Failed to deserialize WebSocket JSON packet");
            }
        }
    }
}

void initWebInterface() {
    WebLog.setWebSocket(&ws);

    const bool hasIndexHtml = LittleFS.exists("/index.html");
    if (!hasIndexHtml) {
        WebLog.println("[WARNING] /index.html not found on LittleFS. Serving fallback page.");
    }

    WiFi.persistent(false);
    WiFi.disconnect(true);
    WiFi.mode(WIFI_AP);
    WiFi.setSleep(false);

    WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
        if (event == ARDUINO_EVENT_WIFI_AP_STACONNECTED) {
            WebLog.printf("[WIFI] Station connected (MAC: %02X:%02X:%02X:%02X:%02X:%02X, AID: %d)\n",
                          info.wifi_ap_staconnected.mac[0], info.wifi_ap_staconnected.mac[1],
                          info.wifi_ap_staconnected.mac[2], info.wifi_ap_staconnected.mac[3],
                          info.wifi_ap_staconnected.mac[4], info.wifi_ap_staconnected.mac[5],
                          info.wifi_ap_staconnected.aid);
        } else if (event == ARDUINO_EVENT_WIFI_AP_STADISCONNECTED) {
            WebLog.printf("[WIFI] Station disconnected (MAC: %02X:%02X:%02X:%02X:%02X:%02X, AID: %d)\n",
                          info.wifi_ap_stadisconnected.mac[0], info.wifi_ap_stadisconnected.mac[1],
                          info.wifi_ap_stadisconnected.mac[2], info.wifi_ap_stadisconnected.mac[3],
                          info.wifi_ap_stadisconnected.mac[4], info.wifi_ap_stadisconnected.mac[5],
                          info.wifi_ap_stadisconnected.aid);
        } else if (event == ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED) {
            WebLog.printf("[WIFI] IP assigned to station: %s\n",
                          IPAddress(info.wifi_ap_staipassigned.ip.addr).toString().c_str());
        }
    });

    if (!WiFi.softAPConfig(localIP, localGW, localSN)) {
        WebLog.println("[ERROR] Failed to configure SoftAP IP address");
    }
    if (!WiFi.softAP(SSID_NAME, SSID_PASS, 1, 0, MAX_SOFTAP_CONNECTIONS)) {
        WebLog.println("[ERROR] Failed to start SoftAP");
    }

    WebLog.printf("[WEB] SoftAP active. SSID: %s | IP: %s | Max stations: %u\n",
                  SSID_NAME, WiFi.softAPIP().toString().c_str(), MAX_SOFTAP_CONNECTIONS);

    dnsServer.start(53, "*", localIP);
    WebLog.println("[WEB] DNS server started: resolving all hostnames to SoftAP IP");

    if (MDNS.begin("cwall")) {
        MDNS.addService("http", "tcp", 80);
        WebLog.println("[WEB] mDNS responder started: http://cwall.local");
    }

    ws.onEvent(onEvent);
    server.addHandler(&ws);

    // Block direct access to internal database file
    server.on("/highscores.dat", HTTP_GET, [](AsyncWebServerRequest* const request) {
        request->send(403, "text/plain", "Forbidden");
    });

    // Provide instant 204 response for favicon requests
    server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest* const request) {
        request->send(204);
    });

    // Captive portal & network connectivity probes for client OS validation
    server.on("/generate_204", HTTP_GET, [](AsyncWebServerRequest* const request) {
        request->send(204);
    });
    server.on("/gen_204", HTTP_GET, [](AsyncWebServerRequest* const request) {
        request->send(204);
    });
    server.on("/connecttest.txt", HTTP_GET, [](AsyncWebServerRequest* const request) {
        request->send(200, "text/plain", "Microsoft Connect Test");
    });
    server.on("/ncsi.txt", HTTP_GET, [](AsyncWebServerRequest* const request) {
        request->send(200, "text/plain", "Microsoft NCSI");
    });
    server.on("/hotspot-detect.html", HTTP_GET, [](AsyncWebServerRequest* const request) {
        request->send(200, "text/html", "<HTML><HEAD><TITLE>Success</TITLE></HEAD><BODY>Success</BODY></HTML>");
    });

    server.on("/", HTTP_GET, [](AsyncWebServerRequest* const request) {
        if (!isHeapSafe(request)) {
            return;
        }
        if (LittleFS.exists("/index.html")) {
            AsyncWebServerResponse* const response = request->beginResponse(LittleFS, "/index.html", "text/html");
            response->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
            request->send(response);
        } else {
            AsyncWebServerResponse* const response = request->beginResponse(200, "text/html",
                "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>cWall - Setup Required</title>"
                "<style>body{font-family:sans-serif;text-align:center;padding:50px;background:#1a1a1a;color:#fff;}"
                "code{background:#333;padding:4px 8px;border-radius:4px;color:#00ffcc;}"
                "a{color:#00ffcc;text-decoration:none;font-weight:bold;margin:0 10px;}</style></head>"
                "<body><h1>cWall Controller</h1>"
                "<p style='color:#ff5555;font-size:1.2em;'>LittleFS Dateisystem ist leer oder /index.html fehlt!</p>"
                "<p>Bitte die Web-Dateien über PlatformIO hochladen mit:<br><br>"
                "<code>pio run --target uploadfs</code></p>"
                "<hr style='margin:30px auto;max-width:400px;border-color:#444;'>"
                "<p><a href='/logs.html'>System-Logs</a> | <a href='/config.html'>Konfiguration</a></p>"
                "</body></html>");
            response->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
            request->send(response);
        }
    });

    server.serveStatic("/", LittleFS, "/")
          .setCacheControl("public, max-age=86400, immutable");

    server.on("/logs.html", HTTP_GET, [](AsyncWebServerRequest* const request) {
        if (!isHeapSafe(request)) {
            return;
        }
        if (LittleFS.exists("/logs.html")) {
            request->send(LittleFS, "/logs.html", "text/html");
        } else {
            request->send(404, "text/plain", "logs.html not found on LittleFS");
        }
    });
    server.on("/logs.js", HTTP_GET, [](AsyncWebServerRequest* const request) {
        if (!isHeapSafe(request)) {
            return;
        }
        if (LittleFS.exists("/logs.js")) {
            request->send(LittleFS, "/logs.js", "application/javascript");
        } else {
            request->send(404, "text/plain", "logs.js not found on LittleFS");
        }
    });
    server.on("/config.html", HTTP_GET, [](AsyncWebServerRequest* const request) {
        if (!isHeapSafe(request)) {
            return;
        }
        if (LittleFS.exists("/config.html")) {
            request->send(LittleFS, "/config.html", "text/html");
        } else {
            request->send(404, "text/plain", "config.html not found on LittleFS");
        }
    });
    server.on("/config.js", HTTP_GET, [](AsyncWebServerRequest* const request) {
        if (!isHeapSafe(request)) {
            return;
        }
        if (LittleFS.exists("/config.js")) {
            request->send(LittleFS, "/config.js", "application/javascript");
        } else {
            request->send(404, "text/plain", "config.js not found on LittleFS");
        }
    });

    server.begin();
    WebLog.println("[WEB] Webserver listening on port 80");
}

void cleanupWebInterface() {
    // 1. Drain pending DNS queries in a quick non-blocking loop
    for (size_t i = 0; i < 4; ++i) {
        dnsServer.processNextRequest();
    }

    const uint32_t now = millis();
    const uint32_t freeHeap = ESP.getFreeHeap();

    // 2. Client health and inactivity audit
    for (AsyncWebSocketClient& client : ws.getClients()) {
        if (client.status() != WS_CONNECTED) {
            continue;
        }

        // Evict congested clients (TCP output queue backlogged)
        if (client.queueLen() > MAX_CLIENT_QUEUE_LEN) {
            WebLog.printf("[WEBSOCKET] Evicting congested client %u (Queue: %u)\n",
                          client.id(), client.queueLen());
            client.close(1008, "Queue Congested");
            unregisterClient(client.id());
            continue;
        }

        // Evict silent clients exceeding inactivity threshold
        const uint32_t lastSeen = getClientLastSeen(client.id());
        if (lastSeen > 0 && (now - lastSeen > CLIENT_INACTIVITY_TIMEOUT_MS)) {
            WebLog.printf("[WEBSOCKET] Evicting inactive client %u (Idle %lu ms)\n",
                          client.id(), static_cast<unsigned long>(now - lastSeen));
            client.close(1000, "Inactivity Timeout");
            unregisterClient(client.id());
            continue;
        }

        // Emergency eviction if system heap is critically low
        if (freeHeap < CRITICAL_HEAP_BYTES) {
            WebLog.printf("[WEBSOCKET] Critical heap (%u bytes)! Evicting client %u\n",
                          freeHeap, client.id());
            client.close(1013, "Emergency Heap Recovery");
            unregisterClient(client.id());
            break;
        }
    }

    // 3. Purge disconnected sockets
    ws.cleanupClients(MAX_WS_CLIENTS);

    // 4. Flush pending log entries ONLY to subscribed clients (e.g. logs.html)
    WebLog.flushPendingLogs([](const uint32_t clientId) -> bool {
        return isLogSubscriber(clientId);
    });
}

void notifyGameStart(const char* const mode) {
    JsonDocument doc;
    doc["action"] = "start game";
    doc["mode"] = mode;
    doc["durationMs"] = gameInstance.getGameDurationMs();

    String output;
    serializeJson(doc, output);
    broadcastWs(output);
}

void updateScore(const uint8_t scoreP1, const uint8_t scoreP2) {
    JsonDocument doc;
    doc["action"] = "update counter";
    doc["scoreP1"] = scoreP1;
    doc["scoreP2"] = scoreP2;
    doc["counterValue"] = scoreP1;

    String output;
    serializeJson(doc, output);
    broadcastWs(output);
}

void notifyGameOver() {
    const bool isDefaultTime = (gameInstance.getGameDurationMs() == 60000);
    JsonDocument doc;
    doc["action"] = "end of game";
    doc["finalScoreP1"] = gameInstance.getScoreP1();
    doc["finalScoreP2"] = gameInstance.getScoreP2();
    doc["isDefaultTime"] = isDefaultTime;

    const std::array<HighscoreEntry, NUM_HIGHSCORES> highscores = gameInstance.getHighscoresCopy(isDefaultTime);
    JsonArray hsArray = doc["highscoreList"].to<JsonArray>();
    for (size_t i = 0; i < NUM_HIGHSCORES; ++i) {
        JsonObject player = hsArray.add<JsonObject>();
        player["playerName"] = highscores[i].name;
        player["counterValue"] = highscores[i].score;
    }

    String output;
    serializeJson(doc, output);
    broadcastWs(output);
}
