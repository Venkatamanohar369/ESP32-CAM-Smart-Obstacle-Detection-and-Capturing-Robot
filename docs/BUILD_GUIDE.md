# Build & Flashing Guide

## 1. Prerequisites
- **PlatformIO Core `6.2.0`** installed in the workspace-local `.pio-venv/` (`Python 3.12.13`).
- Always set `PLATFORMIO_CORE_DIR` to `.pio-core` (or use your global PlatformIO installation if preferred).
- **Hardware Programmers:**
  - `ESP32-CAM-MB` (Micro-USB) for the AI-Thinker ESP32-CAM.
  - Mini-USB / USB-C cable for the Arduino Nano.

## 2. Configuration (Secrets)
1. Copy [`config/config.example.h`](../config/config.example.h) to `config/config.h`.
2. Enter your 2.4 GHz `WIFI_SSID`, `WIFI_PASSWORD`, `TELEGRAM_BOT_TOKEN`, and `TELEGRAM_CHAT_ID`.
3. Never commit `config/config.h` (excluded in `.gitignore`).

## 3. Compile Both Firmware Targets
In PowerShell from the repository root:
```powershell
$env:PLATFORMIO_CORE_DIR = "$PWD\.pio-core"
.\.pio-venv\Scripts\pio.exe run
```
To build individually:
```powershell
.\.pio-venv\Scripts\pio.exe run -e esp32cam
.\.pio-venv\Scripts\pio.exe run -e nanoatmega328
```
*(Use `-e nanoatmega328new` if your Nano uses the new `115200` baud bootloader.)*

## 4. Flashing Instructions

> **SAFETY:** Disconnect the robot's `12V` power supply and unplug external `5.0V` power from the Arduino Nano before connecting USB cables to flash.

1. **Flash ESP32-CAM:**
   - Dock the ESP32-CAM onto the `ESP32-CAM-MB` board and connect via USB.
   - Run: `.\.pio-venv\Scripts\pio.exe run -e esp32cam -t upload`
   - If auto-reset fails, hold `IO0`, tap `RST`, release `IO0` during `Connecting...`.
2. **Flash Arduino Nano:**
   - Ensure the `5.0V` Buck rail is **disconnected** from the Nano `5V` pin.
   - Connect Nano via USB and run: `.\.pio-venv\Scripts\pio.exe run -e nanoatmega328 -t upload` (or `-e nanoatmega328new`).
