#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

ESP8266WebServer server(80);

// Construct HTML string
String getHTML(String head, String body) {
    String style = "body, html { font-family: sans-serif; }";
    return "<!DOCTYPE html><html>"
        "<head>" + head + "<meta charset=\"UTF-8\"><style>" + style + "</style></head>"
        "<body>" + body + "</body>"
        "</html>";
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
    
    String body = "<h2>Welcome to ESP8266</h2>"
        "<p>Uptime: " + uptimeStr + "</p>"
        "<p>User agent: " + userAgent + "</p>";
    String html = getHTML("<title>Home</title>", body);
    server.send(200, "text/html", html);
}

void handleData() {
    String json = "{ \"rot\": [" + String(rot[0]) + ", " + String(rot[1]) + "] }";
    server.sendHeader("Access-Control-Allow-Origin", "http://localhost");
    server.send(200, "application/json", json);
}

void handleNotFound() {
    String body ="<h2>404 - Page not found</h2>"
        "<p>Click <a href=\"/\">here</a> to go to the main page</p>";
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
