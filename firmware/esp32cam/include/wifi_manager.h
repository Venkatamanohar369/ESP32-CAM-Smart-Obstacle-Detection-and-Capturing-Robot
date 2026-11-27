#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include "app_config.h"

class WifiManager {
public:
    void begin() {
        WiFi.mode(WIFI_STA);
        WiFi.setSleep(false); // Prevent Wi-Fi power-save latency spikes during MJPEG streaming

        if (strcmp(WIFI_SSID, "YOUR_WIFI_SSID") == 0 || strlen(WIFI_SSID) == 0) {
            Serial.println(F("[WIFI] WARNING: Placeholder WIFI_SSID in config. Create config/config.h."));
            configured_ = false;
            return;
        }
        configured_ = true;
        connectBlockingLimited();
    }

    void update(uint32_t nowMs) {
        if (!configured_) return;
        if (WiFi.status() == WL_CONNECTED) return;

        if (nowMs - lastReconnectMs_ >= WIFI_RECONNECT_MS) {
            lastReconnectMs_ = nowMs;
            Serial.println(F("[WIFI] Reconnecting..."));
            WiFi.disconnect();
            WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        }
    }

    bool isConnected() const { return WiFi.status() == WL_CONNECTED; }
    bool isConfigured() const { return configured_; }

private:
    bool configured_ = false;
    uint32_t lastReconnectMs_ = 0;

    void connectBlockingLimited() {
        Serial.printf("[WIFI] Connecting to %s", WIFI_SSID);
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        uint32_t start = millis();
        while (WiFi.status() != WL_CONNECTED && (millis() - start) < WIFI_CONNECT_TIMEOUT_MS) {
            delay(250);
            Serial.print('.');
        }
        Serial.println();
        if (WiFi.status() == WL_CONNECTED) {
            Serial.printf("[WIFI] Connected! IP: %s\n", WiFi.localIP().toString().c_str());
        } else {
            Serial.println(F("[WIFI] Initial connect timed out; will retry non-blocking."));
        }
    }
};

#endif // WIFI_MANAGER_H
