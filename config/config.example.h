#ifndef ROBOT_CONFIG_H
#define ROBOT_CONFIG_H

// ============================================================================
// COPY THIS FILE TO `config/config.h` AND FILL IN YOUR REAL CREDENTIALS.
// NEVER COMMIT `config/config.h` TO GIT.
// ============================================================================

// --- Wi-Fi Configuration (2.4 GHz required for ESP32) ---
#define WIFI_SSID               "YOUR_WIFI_SSID"
#define WIFI_PASSWORD           "YOUR_WIFI_PASSWORD"
#define WIFI_CONNECT_TIMEOUT_MS 15000UL
#define WIFI_RECONNECT_MS       10000UL

// --- Telegram Bot API Configuration ---
#define TELEGRAM_ENABLED        false
#define TELEGRAM_BOT_TOKEN      "YOUR_TELEGRAM_BOT_TOKEN"
#define TELEGRAM_CHAT_ID        "YOUR_TELEGRAM_CHAT_ID"
#define TELEGRAM_COOLDOWN_MS    15000UL
#define TELEGRAM_TIMEOUT_MS     10000UL

// Set to false to validate api.telegram.org against the embedded root CA.
// Set to true only for temporary local TLS troubleshooting.
#define TELEGRAM_ALLOW_INSECURE_TLS false

// --- OV3660 Camera Orientation & Quality ---
// Adjust vflip/hmirror (0 or 1) to match your physical camera mounting.
#define CAMERA_VFLIP            1
#define CAMERA_HMIRROR          0

// --- Robot Safety & Sensor Parameters ---
#define OBSTACLE_STOP_CM        20.0f
#define PIR_CAPTURE_COOLDOWN_MS 8000UL
#define UART_BAUD_RATE          38400UL
#define DEFAULT_MOTOR_SPEED     180    // 0..255 PWM
#define MOTOR_CMD_TIMEOUT_MS    1200UL // Stop motors if no heartbeat/refresh

// --- Buzzer Mode ---
// true  = passive buzzer driven with tone() (2 kHz)
// false = active 5V DC buzzer driven HIGH/LOW
#define BUZZER_PASSIVE_MODE     false

#endif // ROBOT_CONFIG_H
