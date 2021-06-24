#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include "frontend.h"

ESP8266WebServer server(80);

// Construct HTML string
String getHTML(String head, String body) {
    String style = "body, html { font-family: sans-serif; }";
    return "<!DOCTYPE html>\n<html>\n"
        "<head>\n" + head + "<meta charset=\"UTF-8\">\n<style>" + style + "</style>\n</head>\n"
        "<body>\n" + body + "</body>\n"
        "</html>\n";
}

// Root page can be accessed only if authentication is ok
void handleRoot() {
    int seconds = millis() / 1000;
    int minutes = seconds / 60;
    int hours = minutes / 60;
    minutes %= 60;
    seconds %= 60;
    
    char uptimeCStr[64];
    snprintf(uptimeCStr, sizeof(uptimeCStr), "%02d:%02d:%02d", hours, minutes, seconds);
    String uptimeStr = String(uptimeCStr);

    String userAgent = server.header("User-Agent");
    
    String body = "<h2>Welcome to ESP8266</h2>\n"
        "<p>Uptime: " + uptimeStr + "</p>\n"
        "<p>User agent: " + userAgent + "</p>\n"
        "<canvas id=\"canvas\" width=\"960\" height=\"540\"></canvas>\n";
    String head = "<title>Home</title>\n"
     "<script type=\"text/javascript\" src=\"script.js\"></script>\n";
    String html = getHTML(head, body);
    server.send(200, "text/html", html);
}

void handleScript() {
    server.send(200, "application/javascript", frontend_script);
}

void handleData() {
    String json = "{ \"rot\": [" + String(rot[0]) + ", " + String(rot[1]) + "] }";
    server.sendHeader("Access-Control-Allow-Origin", "http://localhost");
    server.send(200, "application/json", json);
}

void handleHistory() {
    //String json = "{ \"rot\": [" + String(rot[0]) + ", " + String(rot[1]) + "] }";
    String json = "{ "
        "\"mean\": [" + String(acc_mean[0]) + ", " + String(acc_mean[1]) + ", " + String(acc_mean[2]) + "], "
        "\"dev\": [" + String(acc_dev[0])  + ", " + String(acc_dev[1])  + ", " + String(acc_dev[2])  + "], "
        "\"acc\": [";
    for (int i = 0; i < 3; ++i) {
        json += "[";
        for (int j = 0; j < HISTORY; ++j) {
            json += String(acc_hist[i][(acc_i + j) % HISTORY]);
            if (j < HISTORY - 1) json += ", ";
        }
        json += "]";
        if (i < 2) json += ", ";
    }
    json += "] }";
    server.sendHeader("Access-Control-Allow-Origin", "http://localhost");
    server.send(200, "application/json", json);
}

void handleNotFound() {
    String body ="<h2>404 - Page not found</h2>\n"
        "<p>Click <a href=\"/\">here</a> to go to the main page</p>\n";
    String html = getHTML("<title>Page not found</title>", body);
    server.send(404, "text/html", html);
}

void setupWiFiAP() {
    Serial.print("Setting up access point... ");
    WiFi.mode(WIFI_AP);
    WiFi.softAP(NETWORK_SSID, NETWORK_PASSWORD);
    Serial.println("Done!");
    Serial.print("Local IP: ");
    Serial.println(WiFi.softAPIP());
}

void setupWiFiSTA() {
    Serial.print("Connecting to access point... ");
    WiFi.mode(WIFI_STA);
    WiFi.begin(NETWORK_SSID, NETWORK_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
    }
    Serial.println("Connected!");
    Serial.print("Local IP: ");
    Serial.println(WiFi.localIP());
}

void setupServer() {
    
#if NETWORK_MODE == 0
    setupWiFiAP();
#else if NETWORK_MODE == 1
    setupWiFiSTA();
#endif

    server.on("/", handleRoot);
    server.on("/data", handleData);
    server.on("/history", handleHistory);
    server.on("/script.js", handleScript);
    server.onNotFound(handleNotFound);

    // Headers to track
    const char * headerkeys[] = { "User-Agent", "Cookie" } ;
    size_t headerkeyssize = sizeof(headerkeys) / sizeof(char*);
    server.collectHeaders(headerkeys, headerkeyssize);

    // Start the server
    server.begin();
    Serial.println("HTTP server started");
}

void loopServer() {
    server.handleClient();
}
