# System & Firmware Architecture — ESP32-CAM Autonomous Surveillance Robot

This document details the dual-controller **Option A** architecture (`AI-Thinker ESP32-CAM` + `Arduino Nano`).

---

## 1. Firmware Architecture Diagram

```mermaid
flowchart TB
    subgraph ESP_FW["ESP32-CAM Firmware (firmware/esp32cam/)"]
        MAIN_E["main.cpp<br/>Coordinator Loop"]
        CAM["CameraManager<br/>(OV3660 + Mutex)"]
        CAP["CaptureStore<br/>(6-Slot PSRAM Ring)"]
        WEB["RobotWebServer<br/>(:80 API + :81 MJPEG)"]
        WIFI["WifiManager<br/>(Non-blocking Reconnect)"]
        TG["TelegramNotifier<br/>(Core 0 FreeRTOS Task)"]
        UB["UartBridge<br/>(Serial2 GPIO 13/14)"]
    end

    subgraph SHARED["Shared Protocol & Logic (firmware/shared/)"]
        PROTO["uart_protocol.h<br/>($CMD / $TEL + XOR Checksum)"]
        LOGIC["robot_logic.h<br/>(MotorSafetyState, ObstacleDetector, PirCooldown)"]
    end

    subgraph NANO_FW["Arduino Nano Firmware (firmware/nano/)"]
        MAIN_N["main.cpp<br/>Real-Time Safety Loop"]
        MOT["MotorController<br/>(Soft-Start + 100ms Dead-Time)"]
        US["UltrasonicSensor<br/>(Bounded 25ms Echo Timeout)"]
        PIR["PirSensor<br/>(D12 Input)"]
        BUZ["BuzzerController<br/>(Active/Passive Non-Blocking)"]
    end

    MAIN_E --> CAM & CAP & WEB & WIFI & TG & UB
    UB <-->|"38400 bps Checksummed UART"| MAIN_N
    MAIN_N --> MOT & US & PIR & BUZ
    ESP_FW -.-> SHARED
    NANO_FW -.-> SHARED
```

---

## 2. Communication Flow Diagram

```mermaid
sequenceDiagram
    participant Browser as Web Dashboard
    participant ESP as ESP32-CAM
    participant Nano as Arduino Nano
    participant TG as Telegram API

    Browser->>ESP: POST /api/move?dir=F&speed=180
    ESP->>Nano: $CMD,M,F,180*XX
    Note over Nano: Checks ObstacleDetector<br/>Applies 100ms dead-time & PWM ramp
    Nano-->>ESP: $TEL,450,0,0,F,180*XX (every 100ms)
    Browser->>ESP: GET /api/status
    ESP-->>Browser: JSON {distMm:450, pir:false, obs:false, ...}

    Note over Nano: PIR D12 goes HIGH
    Nano-->>ESP: $TEL,420,1,0,S,0*XX
    Note over ESP: PirCooldown rising-edge trigger
    ESP->>Nano: $CMD,Z,1*XX (Beep)
    ESP->>ESP: Capture OV3660 JPEG + store dist=420mm
    ESP->>TG: Core 0 HTTPS POST /sendPhoto
```

---

## 3. Sensor Event & Safety Flowchart

```mermaid
flowchart TD
    START(["Nano 60ms Sensor Tick"]) --> US_READ["Read HC-SR04 (25ms timeout)"]
    US_READ --> FILT{"2 Consecutive Readings <= 200mm?"}
    FILT -- Yes --> OBS_ON["Set Obstacle Active = TRUE<br/>Force Motor STOP + Buzzer Alarm"]
    FILT -- No --> HYST{"2 Consecutive Readings >= 230mm?"}
    HYST -- Yes --> OBS_OFF["Clear Obstacle = FALSE<br/>Silence Alarm"]
    HYST -- No --> KEEP["Keep Current Obstacle State"]

    OBS_ON & OBS_OFF & KEEP --> TEL["Send $TEL Frame to ESP32-CAM"]
    TEL --> PIR_CHK{"ESP32: PIR Rising Edge & Cooldown Elapsed?"}
    PIR_CHK -- Yes --> CAP_IMG["Acquire OV3660 Frame + Attach Distance<br/>Save to PSRAM Gallery + Queue Telegram"]
    PIR_CHK -- No --> IDLE(["Continue Loop"])
```
