# GPIO Mapping Reference — Option A (VERIFIED)

> **Status:** **USER BENCH-VERIFIED (`2026-11-27`).**  
> **Architecture:** AI-Thinker ESP32-CAM + Arduino Nano (`ATmega328P`, `5V / 16 MHz`) over `38400` bps UART (`Serial2` ↔ `AltSoftSerial`).

---

## 1. AI-Thinker ESP32-CAM Pin Mapping

| Pin | Function | Direction | Conditioning / Notes | Status |
| :--- | :--- | :---: | :--- | :---: |
| `GPIO 14` | `UART2_TX` → Nano `D8` (`RX`) | Out (`3.3V`) | **Via `74AHCT125` / `74HCT125` (`VCC=5.0V`) 3.3V→5.0V buffer** | **PROPOSED** |
| `GPIO 13` | `UART2_RX` ← Nano `D9` (`TX`) | In (`3.3V`) | **Via `1 kΩ` + `2 kΩ` resistor voltage divider (`3.33V`)** | **PROPOSED** |
| `GPIO 4` | Onboard White Flash LED | Out | Kept free; OFF at startup; dashboard toggle | Internal |
| `GPIO 33` | Onboard Red Status LED | Out | Active LOW Wi-Fi heartbeat | Internal |
| `GPIO 1, 3` | `UART0` (`TX`, `RX`) | I/O | Reserved for `ESP32-CAM-MB` flashing & debug | Reserved |
| `GPIO 0` | Camera `XCLK` / Boot Strapping | Out/In | Reserved for OV3660 & `ESP32-CAM-MB` download mode | Reserved |
| `GPIO 16` | PSRAM `CS` | Out | Reserved for onboard PSRAM (OV3660 buffers) | Reserved |
| `GPIO 5,18,19,21,22,23,25,26,27,32,34,35,36,39` | OV3660 Camera Bus | I/O | Reserved for 24-pin FPC camera | Reserved |
| `GPIO 2, 12, 15` | Unused | — | MicroSD disabled (`SD_MMC` not initialized) | Free |

---

## 2. Arduino Nano (`ATmega328P`, 5V) Pin Mapping

| Pin | Function | Peripheral | Conditioning / Notes | Status |
| :--- | :--- | :--- | :--- | :---: |
| `D8` (`ICP1`) | `AltSoftSerial RX` | ESP32 `GPIO 14` | Output of **`74AHCT125`** (`5.0V` push-pull) | **PROPOSED** |
| `D9` (`OC1A`) | `AltSoftSerial TX` | ESP32 `GPIO 13` | To **`1 kΩ` + `2 kΩ` divider** (`5.0V → 3.33V`) | **PROPOSED** |
| `D5` (Timer0 PWM) | `ENA` | L298N Left PWM | `10 kΩ` pull-down to `GND`; remove `ENA` jumper | **PROPOSED** |
| `D6` (Timer0 PWM) | `ENB` | L298N Right PWM | `10 kΩ` pull-down to `GND`; remove `ENB` jumper | **PROPOSED** |
| `D4`, `D7` | `IN1`, `IN2` | L298N Left Dir | `5V` logic | **PROPOSED** |
| `D2`, `D3` | `IN3`, `IN4` | L298N Right Dir | `5V` logic | **PROPOSED** |
| `D10` | `TRIG` | HC-SR04 | `5V` digital output (Timer1 disables PWM on `D10` only) | **PROPOSED** |
| `D11` | `ECHO` | HC-SR04 | `5V` native digital input | **PROPOSED** |
| `D12` | `OUT` | PIR Sensor | Verify PIR `OUT` voltage (`5V` or `3.3V`) | **PROPOSED** |
| `A0` (`D14`) | `BUZZER` | 5V Buzzer | `1 kΩ` → NPN (`2N2222`/`BC547`/`S8050`) + `1N4148` diode | **PROPOSED** |
| `D0`, `D1` | USB Serial | Nano USB | Free for flashing & Nano serial debug | Reserved |
| `D13` | Onboard `L` LED | — | Avoided (blinks during Nano bootloader reset) | Unused |
