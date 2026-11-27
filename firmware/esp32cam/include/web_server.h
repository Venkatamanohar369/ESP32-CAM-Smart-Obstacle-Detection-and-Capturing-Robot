#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <Arduino.h>
#include <WiFi.h>
#include "esp_http_server.h"
#include "app_config.h"
#include "camera_manager.h"
#include "capture_store.h"
#include "uart_bridge.h"
#include "telegram_notifier.h"
#include "web_dashboard.h"

class RobotWebServer {
public:
    typedef void (*CaptureCallback)(const char* reason);

    void begin(CameraManager* cam, CaptureStore* store, UartBridge* uart, TelegramNotifier* tg, CaptureCallback capCb) {
        cam_ = cam;
        store_ = store;
        uart_ = uart;
        tg_ = tg;
        capCb_ = capCb;
        instance_ = this;

        httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
        cfg.server_port = 80;
        cfg.ctrl_port = 32768;
        cfg.max_uri_handlers = 8;

        if (httpd_start(&httpServer_, &cfg) == ESP_OK) {
            registerUri(httpServer_, "/", HTTP_GET, handleIndex);
            registerUri(httpServer_, "/api/status", HTTP_GET, handleStatus);
            registerUri(httpServer_, "/api/move", HTTP_POST, handleMove);
            registerUri(httpServer_, "/api/capture", HTTP_POST, handleCapture);
            registerUri(httpServer_, "/api/buzzer", HTTP_POST, handleBuzzer);
            registerUri(httpServer_, "/api/flash", HTTP_POST, handleFlash);
            registerUri(httpServer_, "/api/photo", HTTP_GET, handlePhoto);
            Serial.println(F("[HTTP] Control server started on port 80."));
        }

        httpd_config_t streamCfg = HTTPD_DEFAULT_CONFIG();
        streamCfg.server_port = 81;
        streamCfg.ctrl_port = 32769;
        if (httpd_start(&streamServer_, &streamCfg) == ESP_OK) {
            registerUri(streamServer_, "/stream", HTTP_GET, handleStream);
            Serial.println(F("[HTTP] MJPEG stream server started on port 81."));
        }
    }

private:
    httpd_handle_t httpServer_ = nullptr;
    httpd_handle_t streamServer_ = nullptr;
    CameraManager* cam_ = nullptr;
    CaptureStore* store_ = nullptr;
    UartBridge* uart_ = nullptr;
    TelegramNotifier* tg_ = nullptr;
    CaptureCallback capCb_ = nullptr;

    static RobotWebServer* instance_;

    static void registerUri(httpd_handle_t srv, const char* uri, httpd_method_t method, esp_err_t (*handler)(httpd_req_t*)) {
        httpd_uri_t def = {};
        def.uri = uri;
        def.method = method;
        def.handler = handler;
        def.user_ctx = nullptr;
        httpd_register_uri_handler(srv, &def);
    }

    static bool getQueryParam(httpd_req_t* req, const char* key, char* out, size_t outSize) {
        size_t qlen = httpd_req_get_url_query_len(req) + 1;
        if (qlen <= 1 || qlen > 128) return false;
        char qbuf[128];
        if (httpd_req_get_url_query_str(req, qbuf, sizeof(qbuf)) != ESP_OK) return false;
        return httpd_query_key_value(qbuf, key, out, outSize) == ESP_OK;
    }

    static esp_err_t handleIndex(httpd_req_t* req) {
        httpd_resp_set_type(req, "text/html");
        return httpd_resp_send(req, WEB_DASHBOARD_HTML, HTTPD_RESP_USE_STRLEN);
    }

    static esp_err_t handleStatus(httpd_req_t* req) {
        RobotWebServer* self = instance_;
        uint32_t nowMs = millis();
        const auto& tel = self->uart_->telemetry();

        CapturedImage meta[CaptureStore::MAX_CAPTURES];
        size_t capCount = self->store_->listMetadata(meta, CaptureStore::MAX_CAPTURES);

        String json = "{";
        json += "\"camOk\":" + String(self->cam_->isInitialized() ? "true" : "false") + ",";
        json += "\"psram\":" + String(self->cam_->hasPsram() ? "true" : "false") + ",";
        json += "\"wifiRssi\":" + String(WiFi.RSSI()) + ",";
        json += "\"nanoOnline\":" + String(self->uart_->isNanoOnline(nowMs) ? "true" : "false") + ",";
        json += "\"distMm\":" + String(tel.distanceMm) + ",";
        json += "\"pir\":" + String(tel.pirTriggered ? "true" : "false") + ",";
        json += "\"obs\":" + String(tel.obstacleActive ? "true" : "false") + ",";
        json += "\"motorDir\":\"" + String(robot_proto::dirToChar(tel.motorDir)) + "\",";
        json += "\"pwm\":" + String(tel.currentPwm) + ",";
        json += "\"tgEnabled\":" + String(self->tg_->isEnabled() ? "true" : "false") + ",";
        json += "\"tgOk\":" + String(self->tg_->lastSendOk() ? "true" : "false") + ",";
        json += "\"tgStatus\":\"" + String(self->tg_->lastStatus()) + "\",";
        json += "\"captures\":[";
        for (size_t i = 0; i < capCount; ++i) {
            if (i > 0) json += ",";
            json += "{\"id\":" + String(meta[i].id) +
                    ",\"tsMs\":" + String(meta[i].timestampMs) +
                    ",\"distMm\":" + String(meta[i].distanceMm) +
                    ",\"reason\":\"" + String(meta[i].reason) + "\"}";
        }
        json += "]}";

        httpd_resp_set_type(req, "application/json");
        httpd_resp_set_hdr(req, "Cache-Control", "no-store");
        return httpd_resp_send(req, json.c_str(), json.length());
    }

    static esp_err_t handleMove(httpd_req_t* req) {
        char dirBuf[8] = {0};
        char spdBuf[8] = {0};
        if (!getQueryParam(req, "dir", dirBuf, sizeof(dirBuf))) {
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing dir");
            return ESP_FAIL;
        }
        char d = dirBuf[0];
        if (d != 'F' && d != 'B' && d != 'L' && d != 'R' && d != 'S') {
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid dir");
            return ESP_FAIL;
        }
        int speed = DEFAULT_MOTOR_SPEED;
        if (getQueryParam(req, "speed", spdBuf, sizeof(spdBuf))) {
            speed = atoi(spdBuf);
            if (speed < 0 || speed > 255) {
                httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid speed");
                return ESP_FAIL;
            }
        }
        instance_->uart_->sendMove(robot_proto::charToDir(d), static_cast<uint8_t>(speed));
        httpd_resp_set_type(req, "application/json");
        return httpd_resp_sendstr(req, "{\"ok\":true}");
    }

    static esp_err_t handleCapture(httpd_req_t* req) {
        if (instance_->capCb_) {
            instance_->capCb_("MANUAL");
        }
        httpd_resp_set_type(req, "application/json");
        return httpd_resp_sendstr(req, "{\"ok\":true}");
    }

    static esp_err_t handleBuzzer(httpd_req_t* req) {
        char modeBuf[8] = {0};
        if (!getQueryParam(req, "mode", modeBuf, sizeof(modeBuf))) {
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing mode");
            return ESP_FAIL;
        }
        int mode = atoi(modeBuf);
        if (mode < 0 || mode > 2) {
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid mode");
            return ESP_FAIL;
        }
        instance_->uart_->sendBuzzer(static_cast<uint8_t>(mode));
        httpd_resp_set_type(req, "application/json");
        return httpd_resp_sendstr(req, "{\"ok\":true}");
    }

    static esp_err_t handleFlash(httpd_req_t* req) {
        char onBuf[8] = {0};
        if (!getQueryParam(req, "on", onBuf, sizeof(onBuf))) {
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing on");
            return ESP_FAIL;
        }
        int on = atoi(onBuf);
        digitalWrite(PIN_FLASH_LED, (on != 0) ? HIGH : LOW);
        httpd_resp_set_type(req, "application/json");
        return httpd_resp_sendstr(req, "{\"ok\":true}");
    }

    static esp_err_t handlePhoto(httpd_req_t* req) {
        char idBuf[16] = {0};
        if (!getQueryParam(req, "id", idBuf, sizeof(idBuf))) {
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing id");
            return ESP_FAIL;
        }
        uint32_t id = static_cast<uint32_t>(strtoul(idBuf, nullptr, 10));
        uint8_t* buf = nullptr;
        size_t len = 0;
        if (!instance_->store_->copyImageById(id, &buf, &len)) {
            httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Not found");
            return ESP_FAIL;
        }
        httpd_resp_set_type(req, "image/jpeg");
        httpd_resp_set_hdr(req, "Cache-Control", "no-store");
        esp_err_t res = httpd_resp_send(req, reinterpret_cast<const char*>(buf), len);
        free(buf);
        return res;
    }

    static esp_err_t handleStream(httpd_req_t* req) {
        static const char* STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=123456789000000000000987654321";
        static const char* STREAM_BOUNDARY     = "\r\n--123456789000000000000987654321\r\n";
        static const char* STREAM_PART         = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

        if (!instance_->cam_->isInitialized()) {
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Camera not ready");
            return ESP_FAIL;
        }

        esp_err_t res = httpd_resp_set_type(req, STREAM_CONTENT_TYPE);
        if (res != ESP_OK) return res;

        char partBuf[64];
        for (;;) {
            camera_fb_t* fb = instance_->cam_->acquireFrame(200);
            if (!fb) {
                vTaskDelay(pdMS_TO_TICKS(30));
                continue;
            }
            res = httpd_resp_send_chunk(req, STREAM_BOUNDARY, strlen(STREAM_BOUNDARY));
            if (res == ESP_OK) {
                size_t hlen = snprintf(partBuf, sizeof(partBuf), STREAM_PART, static_cast<unsigned>(fb->len));
                res = httpd_resp_send_chunk(req, partBuf, hlen);
            }
            if (res == ESP_OK) {
                res = httpd_resp_send_chunk(req, reinterpret_cast<const char*>(fb->buf), fb->len);
            }
            instance_->cam_->releaseFrame(fb);
            if (res != ESP_OK) break;
            vTaskDelay(pdMS_TO_TICKS(35)); // ~25 fps cap, reduces OV3660 thermal load
        }
        return res;
    }
};

RobotWebServer* RobotWebServer::instance_ = nullptr;

#endif // WEB_SERVER_H
