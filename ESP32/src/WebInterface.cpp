#include "WebInterface.h"
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

#include "GameLogic.h" // Wichtig für Zugriff auf Highscores und updateLastHighscoreName
#include "WebLog.h"

extern GameLogic gameInstance;

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

const char* ssid = "C-Wall";
const char* password = "cwallpassword"; // Muss mind. 8 Zeichen lang sein

const IPAddress localIP(192, 168, 1, 1);
const IPAddress localGW(192, 168, 1, 1);
const IPAddress localSN(255, 255, 255, 0);

// Helper for color formatting and parsing
static String formatHexColor(uint32_t color) {
    char buf[8];
    sprintf(buf, "#%06X", color & 0xFFFFFF);
    return String(buf);
}

static uint32_t parseHexColor(const char* hexStr) {
    if (hexStr == nullptr || strlen(hexStr) < 7) return 0;
    if (hexStr[0] == '#') hexStr++;
    return (uint32_t)strtoul(hexStr, NULL, 16);
}

// --- Empfangen von Nachrichten vom Browser ---
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_DATA) {
        AwsFrameInfo *info = (AwsFrameInfo*)arg;
        if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
            WebLog.printf("WebSocket empfangen (Länge: %u)\n", len);
            
            // JSON auspacken
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, data, len); // Verwendet Pointer und Länge direkt
            
            if (!error) {
                // Wenn das Tablet einen neuen Namen schickt
                if (doc["action"] == "set name") {
                    const char* newName = doc["playerName"];
                    const int index = doc["index"];
                    gameInstance.updateHighscoreName(index, newName, gameInstance.settings.gameDurationMs == 60000);
                    
                    // Schicke direkt die aktualisierte Highscore-Liste an alle zurück
                    notifyGameOver(); 
                } else if (doc["action"] == "get_logs") {
                    // Send log history back to the requesting client
                    String logsJson = WebLog.getLogsJson();
                    client->text(logsJson);
                } else if (doc["action"] == "get_config") {
                    JsonDocument configDoc;
                    configDoc["action"] = "config_data";
                    configDoc["gameDurationMs"] = gameInstance.settings.gameDurationMs;
                    configDoc["colorSinglePlayer"] = formatHexColor(gameInstance.settings.colorSinglePlayer);
                    configDoc["colorMultiplayerIdle"] = formatHexColor(gameInstance.settings.colorMultiplayerIdle);
                    configDoc["colorP1"] = formatHexColor(gameInstance.settings.colorP1);
                    configDoc["colorP2"] = formatHexColor(gameInstance.settings.colorP2);
                    configDoc["colorP1Ripple"] = formatHexColor(gameInstance.settings.colorP1Ripple);
                    configDoc["colorP2Ripple"] = formatHexColor(gameInstance.settings.colorP2Ripple);
                    
                    String output;
                    serializeJson(configDoc, output);
                    client->text(output);
                } else if (doc["action"] == "set_config") {
                    if (!doc["gameDurationMs"].isNull()) gameInstance.applyNewDuration(doc["gameDurationMs"]);
                    if (!doc["colorSinglePlayer"].isNull()) gameInstance.settings.colorSinglePlayer = parseHexColor(doc["colorSinglePlayer"]);
                    if (!doc["colorMultiplayerIdle"].isNull()) gameInstance.settings.colorMultiplayerIdle = parseHexColor(doc["colorMultiplayerIdle"]);
                    if (!doc["colorP1"].isNull()) gameInstance.settings.colorP1 = parseHexColor(doc["colorP1"]);
                    if (!doc["colorP2"].isNull()) gameInstance.settings.colorP2 = parseHexColor(doc["colorP2"]);
                    if (!doc["colorP1Ripple"].isNull()) gameInstance.settings.colorP1Ripple = parseHexColor(doc["colorP1Ripple"]);
                    if (!doc["colorP2Ripple"].isNull()) gameInstance.settings.colorP2Ripple = parseHexColor(doc["colorP2Ripple"]);
                    WebLog.println("Neue Konfiguration via WebSocket empfangen und angewendet.");
                } else if (doc["action"] == "reset_highscore") {
                    gameInstance.reset60sHighscore();
                    notifyGameOver(); // Broadcast the empty list
                }
            } else {
                WebLog.println("Fehler beim Parsen der JSON-Nachricht");
            }
        }
    }
}

void initWebInterface() {
    WebLog.setWebSocket(&ws);

    if (!LittleFS.exists("/index.html")) {
        WebLog.println("HTML-File nicht gefunden");
        return;
    }
    
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(localIP, localGW, localSN);
    WiFi.softAP(ssid, password);
    WebLog.print("WLAN Access Point gestartet. IP-Adresse: ");
    WebLog.println(WiFi.softAPIP());

    ws.onEvent(onEvent);
    server.addHandler(&ws);

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(LittleFS, "/index.html", "text/html");
    });
    server.serveStatic("/", LittleFS, "/");

    server.on("/logs.html", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(LittleFS, "/logs.html", "text/html");
    });
    server.on("/logs.js", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(LittleFS, "/logs.js", "application/javascript");
    });
    server.on("/config.html", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(LittleFS, "/config.html", "text/html");
    });
    server.on("/config.js", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(LittleFS, "/config.js", "application/javascript");
    });

    server.begin();
    WebLog.println("Webserver gestartet.");
}

void cleanupWebInterface() {
    ws.cleanupClients();
}

// --- Senden von Nachrichten an den Browser ---

void notifyGameStart(const char* mode) {
    JsonDocument doc;
    doc["action"] = "start game";
    doc["mode"] = mode; // "single" oder "multi"
    doc["durationMs"] = gameInstance.settings.gameDurationMs;
    
    String output;
    serializeJson(doc, output);
    ws.textAll(output); // An alle verbundenen Geräte schicken
}

void updateScore(uint8_t scoreP1, uint8_t scoreP2) {
    JsonDocument doc;
    doc["action"] = "update counter";
    doc["scoreP1"] = scoreP1;
    doc["scoreP2"] = scoreP2;
    doc["counterValue"] = scoreP1; // Für Kompatibilität mit deinem alten JS
    
    String output;
    serializeJson(doc, output);
    ws.textAll(output);
}

void notifyGameOver() {
    JsonDocument doc;
    doc["action"] = "end of game";
    doc["finalScoreP1"] = gameInstance.getScoreP1();
    doc["finalScoreP2"] = gameInstance.getScoreP2();
    doc["isDefaultTime"] = (gameInstance.settings.gameDurationMs == 60000);
    
    const HighscoreEntry* highscores = gameInstance.getHighscores(gameInstance.settings.gameDurationMs == 60000);

    // Highscores Array dynamisch aufbauen
    JsonArray hsArray = doc["highscoreList"].to<JsonArray>();
    for(int i=0; i<10; i++) {
        JsonObject player = hsArray.add<JsonObject>();
        player["playerName"] = highscores[i].name;
        player["counterValue"] = highscores[i].score;
    }
    
    String output;
    serializeJson(doc, output);
    ws.textAll(output);
}