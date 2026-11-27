#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "app_config.h"
#include "robot_logic.h"
#include "camera_manager.h"
#include "capture_store.h"
#include "uart_bridge.h"
#include "wifi_manager.h"
#include "telegram_notifier.h"
#include "web_server.h"

namespace {

CameraManager camera;
CaptureStore captureStore;
UartBridge uartBridge;
WifiManager wifiMgr;
TelegramNotifier telegram;
RobotWebServer webServer;
robot_logic::PirCooldown pirCooldown(PIR_CAPTURE_COOLDOWN_MS);

uint32_t lastConfigSyncMs = 0;
uint32_t lastPingMs = 0;
uint32_t lastLedToggleMs = 0;
bool ledState = false;

void performCaptureAndNotify(const char* reason) {
    if (!camera.isInitialized()) {
        Serial.println(F("[CAP] Skipped: camera not initialized."));
        return;
    }
    camera_fb_t* fb = camera.acquireFrame(400);
    if (!fb) {
        Serial.println(F("[CAP] Frame acquire failed."));
        return;
    }
    int32_t distMm = uartBridge.telemetry().distanceMm;
    uint32_t nowMs = millis();
    uint32_t id = captureStore.saveCapture(fb->buf, fb->len, distMm, reason, nowMs);
    if (id > 0) {
        Serial.printf("[CAP] Saved #%lu (%s, %u bytes, dist=%ld mm)\n",
                      static_cast<unsigned long>(id), reason,
                      static_cast<unsigned>(fb->len), static_cast<long>(distMm));
        telegram.queuePhoto(fb->buf, fb->len, distMm, reason, nowMs);
    }
    camera.releaseFrame(fb);
}

} // namespace

void setup() {
    Serial.begin(115200);
    Serial.println(F("\n[BOOT] ESP32-CAM Autonomous Surveillance Robot"));

    pinMode(PIN_FLASH_LED, OUTPUT);
    digitalWrite(PIN_FLASH_LED, LOW); // Keep 1W Flash LED OFF at startup

    pinMode(PIN_STATUS_LED, OUTPUT);
    digitalWrite(PIN_STATUS_LED, HIGH); // Active LOW LED OFF

    captureStore.begin();
    uartBridge.begin();
    camera.begin(); // Safe if fails; web UI reports Camera ERROR
    wifiMgr.begin();
    telegram.begin();
    webServer.begin(&camera, &captureStore, &uartBridge, &telegram, performCaptureAndNotify);

    // Push initial safety thresholds to Arduino Nano
    uartBridge.sendConfig(static_cast<int>(OBSTACLE_STOP_CM), BUZZER_PASSIVE_MODE);
}

void loop() {
    uint32_t nowMs = millis();

    uartBridge.poll(nowMs);
    wifiMgr.update(nowMs);

    // PIR rising-edge motion capture + buzzer + Telegram alert
    if (pirCooldown.update(uartBridge.telemetry().pirTriggered, nowMs)) {
        Serial.println(F("[PIR] Motion rising edge detected."));
        uartBridge.sendBuzzer(1);
        performCaptureAndNotify("PIR");
    }

    // Periodically sync config & keepalive ping to Nano
    if (nowMs - lastConfigSyncMs >= 5000UL) {
        lastConfigSyncMs = nowMs;
        uartBridge.sendConfig(static_cast<int>(OBSTACLE_STOP_CM), BUZZER_PASSIVE_MODE);
    }
    if (nowMs - lastPingMs >= 500UL) {
        lastPingMs = nowMs;
        uartBridge.sendPing();
    }

    // Status LED heartbeat (1 Hz when Wi-Fi OK, 4 Hz when offline)
    uint32_t blinkMs = wifiMgr.isConnected() ? 500UL : 125UL;
    if (nowMs - lastLedToggleMs >= blinkMs) {
        lastLedToggleMs = nowMs;
        ledState = !ledState;
        digitalWrite(PIN_STATUS_LED, ledState ? LOW : HIGH);
    }

    delay(10);
}
