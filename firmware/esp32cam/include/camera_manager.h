#ifndef CAMERA_MANAGER_H
#define CAMERA_MANAGER_H

#include <Arduino.h>
#include "esp_camera.h"
#include "app_config.h"

class CameraManager {
public:
    bool begin() {
        mutex_ = xSemaphoreCreateMutex();

        camera_config_t config;
        config.ledc_channel = LEDC_CHANNEL_0;
        config.ledc_timer   = LEDC_TIMER_0;
        config.pin_d0       = Y2_GPIO_NUM;
        config.pin_d1       = Y3_GPIO_NUM;
        config.pin_d2       = Y4_GPIO_NUM;
        config.pin_d3       = Y5_GPIO_NUM;
        config.pin_d4       = Y6_GPIO_NUM;
        config.pin_d5       = Y7_GPIO_NUM;
        config.pin_d6       = Y8_GPIO_NUM;
        config.pin_d7       = Y9_GPIO_NUM;
        config.pin_xclk     = XCLK_GPIO_NUM;
        config.pin_pclk     = PCLK_GPIO_NUM;
        config.pin_vsync    = VSYNC_GPIO_NUM;
        config.pin_href     = HREF_GPIO_NUM;
        config.pin_sccb_sda = SIOD_GPIO_NUM;
        config.pin_sccb_scl = SIOC_GPIO_NUM;
        config.pin_pwdn     = PWDN_GPIO_NUM;
        config.pin_reset    = RESET_GPIO_NUM;
        config.xclk_freq_hz = 20000000;
        config.pixel_format = PIXFORMAT_JPEG;

        psramPresent_ = psramFound();
        if (psramPresent_) {
            config.frame_size   = FRAMESIZE_VGA; // 640x480 balanced for OV3660 thermal/Wi-Fi
            config.jpeg_quality = 12;
            config.fb_count     = 2;
            config.grab_mode    = CAMERA_GRAB_LATEST;
        } else {
            config.frame_size   = FRAMESIZE_QVGA;
            config.jpeg_quality = 15;
            config.fb_count     = 1;
            config.grab_mode    = CAMERA_GRAB_WHEN_EMPTY;
        }

        esp_err_t err = esp_camera_init(&config);
        if (err != ESP_OK) {
            lastError_ = err;
            initialized_ = false;
            Serial.printf("[CAM] Init failed: 0x%x\n", err);
            return false;
        }

        sensor_t* s = esp_camera_sensor_get();
        if (s) {
            sensorPid_ = s->id.PID;
            s->set_vflip(s, CAMERA_VFLIP);
            s->set_hmirror(s, CAMERA_HMIRROR);
            if (s->id.PID == OV3660_PID) {
                s->set_brightness(s, 1);
                s->set_saturation(s, -1);
                Serial.println(F("[CAM] Detected OV3660 sensor."));
            } else {
                Serial.printf("[CAM] Sensor PID=0x%04x initialized.\n", sensorPid_);
            }
        }

        initialized_ = true;
        return true;
    }

    camera_fb_t* acquireFrame(uint32_t timeoutMs = 200) {
        if (!initialized_ || !mutex_) return nullptr;
        if (xSemaphoreTake(mutex_, pdMS_TO_TICKS(timeoutMs)) != pdTRUE) {
            return nullptr;
        }
        camera_fb_t* fb = esp_camera_fb_get();
        if (!fb) {
            xSemaphoreGive(mutex_);
            return nullptr;
        }
        return fb;
    }

    void releaseFrame(camera_fb_t* fb) {
        if (fb) {
            esp_camera_fb_return(fb);
        }
        if (mutex_) {
            xSemaphoreGive(mutex_);
        }
    }

    bool isInitialized() const { return initialized_; }
    bool hasPsram() const { return psramPresent_; }
    uint16_t sensorPid() const { return sensorPid_; }
    esp_err_t lastError() const { return lastError_; }

private:
    SemaphoreHandle_t mutex_ = nullptr;
    bool initialized_ = false;
    bool psramPresent_ = false;
    uint16_t sensorPid_ = 0;
    esp_err_t lastError_ = ESP_OK;
};

#endif // CAMERA_MANAGER_H
