#pragma once
#include <WiFi.h>

// Wi-Fi設定
const char* ssid     = "Buffalo-G-7050";
const char* password = "etnxhurnecbs7";


bool INIT_WiFi() {

    Serial.printf("Connecting to %s\n", ssid);
    WiFi.begin(ssid, password);

    int connTimeout = 0;
    while (WiFi.status() != WL_CONNECTED && connTimeout < 40) {
        delay(500);
        Serial.print(".");
        connTimeout++;
    }

    Serial.println("");

    bool isConnect = (WiFi.status() == WL_CONNECTED);
    if (isConnect) {
        Serial.print("WiFi connected. IP: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("WiFi connection failed.");
    }
    return isConnect;
}