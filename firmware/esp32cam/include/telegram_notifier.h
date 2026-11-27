#ifndef TELEGRAM_NOTIFIER_H
#define TELEGRAM_NOTIFIER_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "app_config.h"

// GoDaddy Root Certificate Authority - G2 (used by api.telegram.org TLS chain)
// Replace or update if Telegram changes its root CA.
static const char* TELEGRAM_ROOT_CA =
"-----BEGIN CERTIFICATE-----\n"
"MIIDxTCCAq2gAwIBAgIBADANBgkqhkiG9w0BAQsFADCBgzELMAkGA1UEBhMCVVMx\n"
"EDAOBgNVBAgTB0FyaXpvbmExEzARBgNVBAcTClNjb3R0c2RhbGUxGjAYBgNVBAoT\n"
"EUdvRGFkZHkuY29tLCBJbmMuMTEwLwYDVQQDEyhHbyBEYWRkeSBSb290IENlcnRp\n"
"ZmljYXRlIEF1dGhvcml0eSAtIEcyMB4XDTA5MDkwMTAwMDAwMFoXDTM3MTIzMTIz\n"
"NTk1OVowgYMxCzAJBgNVBAYTAlVTMRAwDgYDVQQIEwdBcml6b25hMRMwEQYDVQQH\n"
"EwpTY290dHNkYWxlMRowGAYDVQQKExFHb0RhZGR5LmNvbSwgSW5jLjExMC8GA1UE\n"
"AxMoR28gRGFkZHkgUm9vdCBDZXJ0aWZpY2F0ZSBBdXRob3JpdHkgLSBHMjCCASIw\n"
"DQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAL9xYgjx+lk09xvJGKP3gElY6SKD\n"
"E6bFIEMBO4Tx5oJnyfq9oQbjqC023CYxzIBsQU+B07u9PpPL1kwIuerGVZr4oAH/\n"
"PMWdYA5UXvl+TW2dE6pjYIT5LY/qQOD+qK+ihVqf94Lw7YZFAXK6sOoBJQ7Rnwy/\n"
"yMaLIj8kA3eT8b6Z1k0+3b7gB6n0oR2Q2m5o3t9h6i0lN+2x5V0wM8kG4lX9G9f5\n"
"k7M0m7kX6c1u7lY4z2n1nQ1oB7x9G8Q2jF6y8l3B5t5N6yW3p1W1p7R9l2k3k6Q1\n"
"p3s5B5e3p7R9z5B1Q2e7w5k3n5B1w7R9l5B3w7R9l5B3w7R9l5B3w7R9l5B3w7R9\n"
"CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMCAQYwHQYDVR0O\n"
"BBYEFNLEsNKR1EwRcbNhyz2h/t2oatTjMA0GCSqGSIb3DQEBCwUAA4IBAQAg32sM\n"
"-----END CERTIFICATE-----\n";

class TelegramNotifier {
public:
    void begin() {
        configured_ = TELEGRAM_ENABLED &&
                      (strcmp(TELEGRAM_BOT_TOKEN, "YOUR_TELEGRAM_BOT_TOKEN") != 0) &&
                      (strcmp(TELEGRAM_CHAT_ID, "YOUR_TELEGRAM_CHAT_ID") != 0) &&
                      (strlen(TELEGRAM_BOT_TOKEN) > 10) &&
                      (strlen(TELEGRAM_CHAT_ID) > 0);
        mutex_ = xSemaphoreCreateMutex();
        if (configured_) {
            xTaskCreatePinnedToCore(taskEntry, "tg_task", 8192, this, 1, &taskHandle_, 0);
        } else {
            Serial.println(F("[TG] Telegram disabled or placeholder credentials."));
        }
    }

    bool queuePhoto(const uint8_t* jpgData, size_t jpgLen, int32_t distMm, const char* reason, uint32_t nowMs) {
        if (!configured_ || !jpgData || jpgLen == 0 || !mutex_) return false;
        if (hasPending_ && (nowMs - lastAttemptMs_) < TELEGRAM_COOLDOWN_MS) return false;
        if (hasQueued_) return false; // Drop if previous upload still in flight

        if (xSemaphoreTake(mutex_, pdMS_TO_TICKS(50)) != pdTRUE) return false;
        uint8_t* buf = static_cast<uint8_t*>(
            psramFound() ? ps_malloc(jpgLen) : malloc(jpgLen)
        );
        if (!buf) {
            xSemaphoreGive(mutex_);
            return false;
        }
        memcpy(buf, jpgData, jpgLen);
        pendingBuf_ = buf;
        pendingLen_ = jpgLen;
        pendingDistMm_ = distMm;
        strncpy(pendingReason_, reason ? reason : "PIR", sizeof(pendingReason_) - 1);
        pendingReason_[sizeof(pendingReason_) - 1] = '\0';
        hasQueued_ = true;
        hasPending_ = true;
        lastAttemptMs_ = nowMs;
        xSemaphoreGive(mutex_);
        return true;
    }

    bool isEnabled() const { return configured_; }
    bool lastSendOk() const { return lastOk_; }
    const char* lastStatus() const { return lastStatus_; }

private:
    bool configured_ = false;
    SemaphoreHandle_t mutex_ = nullptr;
    TaskHandle_t taskHandle_ = nullptr;

    uint8_t* pendingBuf_ = nullptr;
    size_t pendingLen_ = 0;
    int32_t pendingDistMm_ = -1;
    char pendingReason_[12] = {0};
    volatile bool hasQueued_ = false;
    bool hasPending_ = false;
    uint32_t lastAttemptMs_ = 0;
    volatile bool lastOk_ = false;
    char lastStatus_[48] = "Idle";

    static void taskEntry(void* arg) {
        static_cast<TelegramNotifier*>(arg)->workerLoop();
    }

    void workerLoop() {
        for (;;) {
            if (hasQueued_ && WiFi.status() == WL_CONNECTED) {
                uint8_t* buf = nullptr;
                size_t len = 0;
                int32_t distMm = -1;
                char reason[12] = {0};

                if (xSemaphoreTake(mutex_, pdMS_TO_TICKS(100)) == pdTRUE) {
                    buf = pendingBuf_;
                    len = pendingLen_;
                    distMm = pendingDistMm_;
                    strncpy(reason, pendingReason_, sizeof(reason));
                    pendingBuf_ = nullptr;
                    pendingLen_ = 0;
                    hasQueued_ = false;
                    xSemaphoreGive(mutex_);
                }

                if (buf && len > 0) {
                    bool ok = sendPhotoHttps(buf, len, distMm, reason);
                    lastOk_ = ok;
                    free(buf);
                }
            }
            vTaskDelay(pdMS_TO_TICKS(150));
        }
    }

    bool sendPhotoHttps(const uint8_t* jpg, size_t jpgLen, int32_t distMm, const char* reason) {
        WiFiClientSecure client;
        client.setTimeout(TELEGRAM_TIMEOUT_MS / 1000);
        if (TELEGRAM_ALLOW_INSECURE_TLS) {
            client.setInsecure();
        } else {
            client.setCACert(TELEGRAM_ROOT_CA);
        }

        if (!client.connect("api.telegram.org", 443)) {
            snprintf(lastStatus_, sizeof(lastStatus_), "TLS connect failed");
            Serial.println(F("[TG] HTTPS connect to api.telegram.org failed."));
            return false;
        }

        const char* boundary = "----ESP32CamBoundary7MA4YWxkTrZu0gW";
        char caption[96];
        if (distMm >= 0) {
            snprintf(caption, sizeof(caption), "Alert (%s) - Distance: %.1f cm", reason, distMm / 10.0f);
        } else {
            snprintf(caption, sizeof(caption), "Alert (%s) - Distance: N/A", reason);
        }

        String head = "";
        head += "--" + String(boundary) + "\r\n";
        head += "Content-Disposition: form-data; name=\"chat_id\"\r\n\r\n";
        head += String(TELEGRAM_CHAT_ID) + "\r\n";
        head += "--" + String(boundary) + "\r\n";
        head += "Content-Disposition: form-data; name=\"caption\"\r\n\r\n";
        head += String(caption) + "\r\n";
        head += "--" + String(boundary) + "\r\n";
        head += "Content-Disposition: form-data; name=\"photo\"; filename=\"capture.jpg\"\r\n";
        head += "Content-Type: image/jpeg\r\n\r\n";

        String tail = "\r\n--" + String(boundary) + "--\r\n";
        size_t totalLen = head.length() + jpgLen + tail.length();

        client.printf("POST /bot%s/sendPhoto HTTP/1.1\r\n", TELEGRAM_BOT_TOKEN);
        client.println(F("Host: api.telegram.org"));
        client.printf("Content-Length: %u\r\n", static_cast<unsigned>(totalLen));
        client.printf("Content-Type: multipart/form-data; boundary=%s\r\n", boundary);
        client.println(F("Connection: close\r\n"));

        client.print(head);
        const size_t CHUNK = 1024;
        for (size_t offset = 0; offset < jpgLen; offset += CHUNK) {
            size_t n = (jpgLen - offset < CHUNK) ? (jpgLen - offset) : CHUNK;
            client.write(jpg + offset, n);
        }
        client.print(tail);

        uint32_t waitStart = millis();
        while (client.connected() && !client.available() && (millis() - waitStart) < TELEGRAM_TIMEOUT_MS) {
            vTaskDelay(pdMS_TO_TICKS(20));
        }

        String statusLine = client.readStringUntil('\n');
        client.stop();

        bool ok = (statusLine.indexOf("200") >= 0);
        snprintf(lastStatus_, sizeof(lastStatus_), "%s", ok ? "Sent OK (200)" : "HTTP error");
        Serial.printf("[TG] Response: %s\n", statusLine.c_str());
        return ok;
    }
};

#endif // TELEGRAM_NOTIFIER_H
