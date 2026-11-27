#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#if __has_include("../../../config/config.h")
#include "../../../config/config.h"
#else
#include "../../../config/config.example.h"
#endif

// ============================================================================
// AI-Thinker ESP32-CAM Camera Pin Map (OV3660 / OV2640 24-pin FPC)
// Source: AI-Thinker ESP32-CAM Schematic & Espressif esp32-camera
// ============================================================================
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

// ============================================================================
// PROPOSED Option A External Pins (docs/GPIO_FEASIBILITY.md)
// ============================================================================
#define PIN_UART2_RX      13  // From Nano D9 TX via 1k + 2k divider
#define PIN_UART2_TX      14  // To Nano D8 RX via 74AHCT125/74HCT125 buffer
#define PIN_FLASH_LED      4  // Onboard white Flash LED (kept free / off at boot)
#define PIN_STATUS_LED    33  // Onboard red LED (active LOW)

#endif // APP_CONFIG_H
