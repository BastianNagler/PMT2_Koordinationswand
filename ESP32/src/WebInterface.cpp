#include "WebInterface.h"
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

#include "GameLogic.h" // Wichtig für Zugriff auf Highscores und updateLastHighscoreName

extern GameLogic gameInstance;

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

const char* ssid = "C-Wall";
const char* password = "cwallpassword"; // Muss mind. 8 Zeichen lang sein

const IPAddress localIP(192, 168, 1, 1);
const IPAddress localGW(192, 168, 1, 1);
const IPAddress localSN(255, 255, 255, 0);

// --- Empfangen von Nachrichten vom Browser ---
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_DATA) {
        AwsFrameInfo *info = (AwsFrameInfo*)arg;
        if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
            Serial.printf("WebSocket empfangen (Länge: %u)\n", len);
            
            // JSON auspacken
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, data, len); // Verwendet Pointer und Länge direkt
            
            if (!error) {
                // Wenn das Tablet einen neuen Namen schickt
                if (doc["action"] == "set name") {
                    const char* newName = doc["playerName"];
                    const int index = doc["index"];
                    gameInstance.updateHighscoreName(index, newName);
                    
                    // Schicke direkt die aktualisierte Highscore-Liste an alle zurück
                    notifyGameOver(); 
                }
            } else {
                Serial.println("Fehler beim Parsen der JSON-Nachricht");
            }
        }
    }
}

void initWebInterface() {
    if (!LittleFS.exists("/index.html")) {
        Serial.println("HTML-File nicht gefunden");
        return;
    }
    
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(localIP, localGW, localSN);
    WiFi.softAP(ssid, password);
    Serial.print("WLAN Access Point gestartet. IP-Adresse: ");
    Serial.println(WiFi.softAPIP());

    ws.onEvent(onEvent);
    server.addHandler(&ws);

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(LittleFS, "/index.html", "text/html");
    });
    server.serveStatic("/", LittleFS, "/");

    server.begin();
    Serial.println("Webserver gestartet.");
}

void cleanupWebInterface() {
    ws.cleanupClients();
}

// --- Senden von Nachrichten an den Browser ---

void notifyGameStart(const char* mode) {
    JsonDocument doc;
    doc["action"] = "start game";
    doc["mode"] = mode; // "single" oder "multi"
    
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
    
    const HighscoreEntry* highscores = gameInstance.getHighscores();

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