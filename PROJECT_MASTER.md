# PROJECT_MASTER.md — ESP32-CAM Autonomous Surveillance Robot

> **SINGLE SOURCE OF TRUTH** for the complete project state. Maintained continuously per [`AGENTS.md`](AGENTS.md).  
> **Last Updated:** 2026-11-27

---

## 1. Project Identity
- **Project Name:** ESP32-CAM Smart Obstacle Detection and Capturing Robot (ESP32-CAM Autonomous Surveillance Robot)
- **Demo Video:** [Esp32 cam based obstacle detection and capturing robot](https://www.youtube.com/watch?v=9W-vfZcfwpw)
- **Purpose:** Browser-controlled wireless surveillance robot with live MJPEG video streaming, PIR human-motion image capture, ultrasonic distance measurement, emergency forward obstacle braking, buzzer alerts, and Telegram photo notifications.
- **Problem Statement:** The original prototype's firmware and wiring documentation were lost ([`source_material/PROJECT_BRIEF.md`](source_material/PROJECT_BRIEF.md)). Additionally, the AI-Thinker ESP32-CAM lacks enough conflict-free GPIOs for 4 L298N direction lines, PIR, HC-SR04, and buzzer without sacrificing `UART0` (`ESP32-CAM-MB`) and `GPIO 4` (1W Flash LED).
- **Objectives:** Clean-room dual-controller re-implementation (`Option A`: ESP32-CAM + Arduino Nano) with verified software compilation, static/unit/integration tests, safe power/level-shifting design, and reproducible builds.
- **Current Development Stage:** **100% Complete — Firmware Built, Automated Software Tests Passed (`10/10`), Physical Hardware User-Verified (`2026-11-27`), and Published to GitHub.**
- **Repository Structure:** See Section 16.

---

## 2. Complete Project Overview
1. **Startup:** Arduino Nano initializes all motor pins (`ENA`, `ENB`, `IN1–IN4`) to `STOP` (`0` PWM, `LOW`) and enables `AltSoftSerial` (`38400` bps on `D8`/`D9`). ESP32-CAM keeps `GPIO 4` Flash LED `OFF`, initializes `CaptureStore` (6-slot PSRAM ring buffer), `Serial2` (`GPIO 13`/`14`), OV3660 camera, Wi-Fi STA, async Core-0 `TelegramNotifier`, and dual HTTP servers (port `80` API/dashboard, port `81` MJPEG).
2. **Normal Operation:**
   - Arduino Nano polls HC-SR04 every `60 ms` (bounded `25 ms` echo timeout) and reads PIR (`D12`), streaming `$TEL` packets to ESP32-CAM every `100 ms`.
   - Browser users view live MJPEG (`:81/stream`) and control motion (`F/B/L/R/S`) via REST `/api/move`, which forwards `$CMD,M,...` over UART.
   - Nano applies `100 ms` direction-reversal dead-time, PWM soft-start ramping, and a `1200 ms` command watchdog timeout.
3. **Obstacle Emergency Stop:** Two consecutive ultrasonic readings `≤ OBSTACLE_STOP_CM` (`20 cm` default) trigger emergency stop (`FORWARD` blocked, `BACKWARD`/`LEFT`/`RIGHT`/`STOP` allowed) and a pulsed 2 Hz buzzer alarm until cleared with hysteresis (`≥ 23 cm`).
4. **Motion Capture & Telegram:** On a PIR rising edge (outside `8000 ms` cooldown), ESP32-CAM triggers a short buzzer beep, captures an OV3660 JPEG frame paired with the latest ultrasonic distance, stores it in the PSRAM gallery, and queues an async Core-0 HTTPS `POST /sendPhoto` to Telegram (when enabled).

---

## 3. Hardware Inventory

| Component | Qty | Confirmed Specs | Estimated / Unverified Specs | Physical Verification Needed |
| :--- | :---: | :--- | :--- | :--- |
| **AI-Thinker ESP32-CAM** | 1 | ESP32-S, 24-pin FPC, `3.3V` logic, `5.0V` in | PSRAM size (`4 MB` typical) | Boot `psramFound()` check |
| **OV3660 Camera** | 1 | 3MP 24-pin 0.5 mm FPC, SCCB + DVP | Lens FOV, `vflip`/`hmirror` orientation | Boot `OV3660_PID` (`0x3660`) check |
| **ESP32-CAM-MB** | 1 | CH340C USB-UART + `IO0`/`RST` buttons | — | Flash/serial test |
| **Arduino Nano** | 1 | ATmega328P, `5V` / `16 MHz` | Bootloader baud (`57600` vs `115200`) | USB upload test |
| **PIR Motion Sensor** | 1 | Digital motion detector | Model (`HC-SR501` vs clone), `OUT` `3.3V` vs `5.0V` | Measure `OUT` voltage at `5V VCC` |
| **HC-SR04 Ultrasonic** | 1 | `5V` ultrasonic distance sensor | Variant (`HC-SR04` vs `HC-SR04P`) | Echo response test |
| **L298N Motor Driver** | 1 | Dual H-bridge, `2 A/ch` DC max, onboard `78M05` | H-bridge drop (`~1.8–3.2 V` est.) | Remove `5V-EN` jumper; thermal check |
| **Single-Shaft BO Motors** | 2 | Rated `6–12 V` | **Winding resistance & stall current UNKNOWN** | Measure $R_w$ / bench stall current |
| **Small 5V Buzzer** | 1 | `5V` nominal | **Active DC vs Passive piezo/magnetic UNKNOWN** | Brief 5V polarity test |
| **12V, 3A DC Adapter** | 1 | `12.0 V`, `3.0 A` (`36 W`) | Load droop & polarity | Multimeter polarity & voltage check |
| **Buck Converter** | 1 | Step-down DC-DC | **Exact IC/model & current rating UNKNOWN** | Confirm `≥ 2.0 A` & tune `5.00V ± 0.10V` |
| **74AHCT125 / 74HCT125** | 1 | 5V TTL-input buffer (`VIH ≥ 2.0V`) | **Additional purchase** | Verify wiring (`GPIO 14 → Nano D8`) |
| **Resistors, Caps, NPN, Diode, Fuse, Switch** | Kit + Purchase | `1kΩ/2kΩ/10kΩ`, `470–1000µF 25V`, `470µF 16V`, `100nF`, `2N2222`/`BC547`/`S8050`, `1N4148`, `3A` fuse (**PROVISIONAL**) | Kit values **UNVERIFIED** | Inventory & multimeter check; see [`hardware/BOM.md`](hardware/BOM.md) |

---

## 4. System Architecture
- **ESP32-CAM Responsibilities:** OV3660 DVP/PSRAM capture (`CameraManager`), 6-slot PSRAM gallery (`CaptureStore`), Wi-Fi STA + non-blocking reconnect (`WifiManager`), HTTP REST API `:80` + MJPEG `:81` (`RobotWebServer`), async Core-0 HTTPS photo alerts (`TelegramNotifier`), and `Serial2` link (`UartBridge`).
- **Arduino Nano Responsibilities:** Real-time HC-SR04 ranging (`UltrasonicSensor`), 2-sample obstacle hysteresis (`ObstacleDetector`), emergency forward motor stop, `100 ms` reversal dead-time + PWM ramping (`MotorController`), PIR digital input (`PirSensor`), active/passive buzzer (`BuzzerController`), and `AltSoftSerial` telemetry/command loop.
- **System Block Diagram:** See [`hardware/block_diagram/SYSTEM_BLOCK_DIAGRAM.md`](hardware/block_diagram/SYSTEM_BLOCK_DIAGRAM.md) and [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md).

---

## 5. GPIO Mapping (`PROPOSED` — Unverified Physically)
Authoritative tables: [`docs/GPIO_FEASIBILITY.md`](docs/GPIO_FEASIBILITY.md) and [`hardware/GPIO_MAPPING.md`](hardware/GPIO_MAPPING.md).

- **ESP32-CAM:** `GPIO 14` (`UART2_TX` → `74AHCT125` → Nano `D8`), `GPIO 13` (`UART2_RX` ← `1kΩ/2kΩ` divider ← Nano `D9`), `GPIO 4` (Flash LED, kept OFF at boot), `GPIO 33` (Red LED heartbeat), `GPIO 1/3` (`UART0` reserved for `ESP32-CAM-MB`), `GPIO 0` (Camera `XCLK`/Boot), `GPIO 16` (PSRAM `CS`), `GPIO 5,18,19,21,22,23,25,26,27,32,34,35,36,39` (OV3660), MicroSD disabled.
- **Arduino Nano:** `D8` (`AltSoftSerial RX`), `D9` (`AltSoftSerial TX`), `D5` (`ENA` PWM + `10kΩ` pull-down), `D6` (`ENB` PWM + `10kΩ` pull-down), `D4`/`D7` (`IN1`/`IN2`), `D2`/`D3` (`IN3`/`IN4`), `D10` (`HC-SR04 TRIG`), `D11` (`HC-SR04 ECHO`), `D12` (`PIR OUT`), `A0` (`Buzzer` via `1kΩ` + NPN + `1N4148`), `D0/D1` (USB Serial), `D13` (unused).

---

## 6. Wiring and Power Design (`PROPOSED` — Power-Up Blocked)
Detailed specs & diagrams: [`docs/POWER_DESIGN.md`](docs/POWER_DESIGN.md), [`hardware/wiring_diagram/WIRING_DIAGRAM.md`](hardware/wiring_diagram/WIRING_DIAGRAM.md), [`hardware/power_distribution/POWER_DISTRIBUTION.md`](hardware/power_distribution/POWER_DISTRIBUTION.md).
- **12V Bus:** `12V 3A` adapter → **`3.0 A` Fast-Blow Fuse (PROVISIONAL)** → SPST Switch → `470–1000 µF 25V` + `100 nF` → L298N `+12V` (`5V-EN` jumper **REMOVED**) & Buck Converter (`≥ 2.0 A` continuous, tuned to `5.00V ± 0.10V` unloaded).
- **5.0V Star Rail:** `~272–393 mA` typical / `~570–720 mA` peak (excl. Flash LED) → ESP32-CAM `5V` (+ local `470 µF 16V` + `100 nF`), Nano `5V` (unplug during USB!), L298N `+5V` (`VSS`), `74AHCT125 VCC`, PIR `VCC`, HC-SR04 `VCC`, Buzzer `+`.
- **Safety Rule:** Never connect `12V` to ESP32-CAM, Nano, `74AHCT125`, or 5V sensors. Complete the Pre-Power-Up Checklist in [`docs/POWER_DESIGN.md`](docs/POWER_DESIGN.md) before energizing.

---

## 7. Firmware Architecture

| Module | File | Responsibility | Interfaces |
| :--- | :--- | :--- | :--- |
| Shared Protocol | [`firmware/shared/include/uart_protocol.h`](firmware/shared/include/uart_protocol.h) | `$CMD`/`$TEL` XOR-checksummed framing & parsing | ESP32 ↔ Nano UART |
| Shared Logic | [`firmware/shared/include/robot_logic.h`](firmware/shared/include/robot_logic.h) | `ObstacleDetector`, `MotorSafetyState`, `PirCooldown`, `echoUsToMm` | Both MCUs |
| ESP32 Main | [`firmware/esp32cam/src/main.cpp`](firmware/esp32cam/src/main.cpp) | System coordinator, PIR capture trigger, heartbeat | All ESP32 modules |
| Camera | [`firmware/esp32cam/include/camera_manager.h`](firmware/esp32cam/include/camera_manager.h) | OV3660 init, PSRAM config, mutex-guarded frame grab | `esp_camera.h` |
| Gallery Store | [`firmware/esp32cam/include/capture_store.h`](firmware/esp32cam/include/capture_store.h) | 6-slot PSRAM JPEG ring buffer + distance/timestamp | `RobotWebServer` |
| Web Server & UI | [`web_server.h`](firmware/esp32cam/include/web_server.h), [`web_dashboard.h`](firmware/esp32cam/include/web_dashboard.h) | `:80` REST API + `PROGMEM` UI, `:81` MJPEG stream | Browser LAN |
| Telegram | [`firmware/esp32cam/include/telegram_notifier.h`](firmware/esp32cam/include/telegram_notifier.h) | Core-0 async `WiFiClientSecure` multipart `/sendPhoto` | `api.telegram.org:443` |
| Wi-Fi & UART | [`wifi_manager.h`](firmware/esp32cam/include/wifi_manager.h), [`uart_bridge.h`](firmware/esp32cam/include/uart_bridge.h) | Non-blocking Wi-Fi reconnect & `Serial2` (`13`/`14`) | Router / Nano |
| Nano Main | [`firmware/nano/src/main.cpp`](firmware/nano/src/main.cpp) | `60 ms` sensor loop, `100 ms` `$TEL`, `$CMD` handler | `AltSoftSerial` (`8`/`9`) |
| Nano Drivers | [`motor_controller.h`](firmware/nano/include/motor_controller.h), [`ultrasonic_sensor.h`](firmware/nano/include/ultrasonic_sensor.h), [`pir_sensor.h`](firmware/nano/include/pir_sensor.h), [`buzzer_controller.h`](firmware/nano/include/buzzer_controller.h) | L298N PWM/dir, HC-SR04, PIR, active/passive buzzer | Nano GPIOs |

---

## 8. Communication Protocol (`38400` bps UART, `$<payload>*<XX>\n`)
- **Validation:** 8-bit XOR checksum over payload bytes (`$` to `*`); malformed or checksum-mismatched frames are discarded.
- **ESP32 → Nano:** `$CMD,M,<F|B|L|R|S>,<0..255>*XX`, `$CMD,Z,<0|1|2>*XX`, `$CMD,CFG,<obs_cm>,<passive_buz>*XX`, `$CMD,PING*XX` (every `500 ms`).
- **Nano → ESP32 (every `100 ms`):** `$TEL,<dist_mm>,<pir_0|1>,<obs_0|1>,<F|B|L|R|S>,<pwm_0..255>*XX` (`dist_mm = -1` on echo timeout/out-of-range).

---

## 9. Software & Dependencies
- **PlatformIO Core:** `6.2.0` (recommended in local Python `3.12` venv via `uv`).
- **ESP32 Platform:** `espressif32 @ 6.9.0` (`framework-arduinoespressif32 @ 3.20017.241212`, `esp_camera`, `esp_http_server`, `WiFi`, `WiFiClientSecure`).
- **AVR Platform:** `atmelavr @ 5.1.0` (`framework-arduino-avr @ 5.2.0`), `paulstoffregen/AltSoftSerial @ ^1.4` (`1.4.0`).
- **Dashboard & Tests:** Vanilla HTML5/CSS3/JS (zero external CDNs), Python 3 `unittest`.

---

## 10. Implemented Features (Code-Verified)
- OV3660 MJPEG streaming (`:81/stream`) and mobile-friendly dashboard (`:80/`)
- Wireless `F/B/L/R/S` + speed slider control with input validation (`400`/`404` checks)
- Real-time HC-SR04 ranging + 2-sample hysteresis emergency forward stop
- Motor `100 ms` reversal dead-time, PWM soft-start, and `1200 ms` watchdog timeout
- PIR rising-edge capture with `8000 ms` cooldown, 6-slot PSRAM gallery with distance & uptime labels
- Non-blocking active/passive 5V buzzer driver & asynchronous Core-0 Telegram photo upload

---

## 11. Planned Features (NOT Implemented)
- 3S LiPo battery + BMS untethered power pack
- Captive-portal Wi-Fi/threshold provisioning in NVS (`Preferences`)
- MOSFET motor driver upgrade (`TB6612FNG`)
- *(No AI object detection, face recognition, SLAM, cloud storage, or servo tracking is implemented.)*

---

## 12. Installation, Build & Test Instructions
See [`docs/BUILD_GUIDE.md`](docs/BUILD_GUIDE.md) and [`tests/test_plan.md`](tests/test_plan.md):
```powershell
uv venv .pio-venv --python 3.12
uv pip install --python .pio-venv\Scripts\python.exe platformio
$env:PLATFORMIO_CORE_DIR = "$PWD\.pio-core"
.\.pio-venv\Scripts\pio.exe run
python -B -m unittest discover -s tests -p "test_*.py" -v
```

---

## 13. Configuration
Copy [`config/config.example.h`](config/config.example.h) to `config/config.h` (gitignored) to set `WIFI_SSID`, `WIFI_PASSWORD`, `TELEGRAM_ENABLED`, `TELEGRAM_BOT_TOKEN`, `TELEGRAM_CHAT_ID`, `CAMERA_VFLIP`/`HMIRROR`, `OBSTACLE_STOP_CM`, `BUZZER_PASSIVE_MODE`. See [`software/telegram/TELEGRAM_SETUP.md`](software/telegram/TELEGRAM_SETUP.md).

---

## 14. Testing & Validation
Full details in [`docs/TEST_REPORT.md`](docs/TEST_REPORT.md):
- **Firmware Builds:** `esp32cam` (`PASS`, RAM `15.5%`, Flash `32.3%`) & `nanoatmega328` (`PASS`, RAM `30.9%`, Flash `28.4%`).
- **Automated Tests (`unittest`):** `10/10 PASS` (static security, pin consistency, protocol, ultrasonic math, dashboard/HTTP/Telegram checks).
- **Physical Hardware Tests:** **`PASS (User-Confirmed Bench Test, 2026-11-27)`** — exact numeric bench measurements not logged in this session.

---

## 15. Known Issues and Risks
- Unknown physical specs: BO motor stall current, Buck Converter rating (`≥2A` required), buzzer active/passive type, PIR `OUT` voltage, kit capacitor/resistor values ([`docs/KNOWN_UNKNOWNS.md`](docs/KNOWN_UNKNOWNS.md)).
- `3.0 A` fuse is **provisional** pending time-current curve and wire-gauge verification.
- MicroSD card slot is disabled (`GPIO 13`/`14` used for `Serial2`).

---

## 16. Repository File Structure
- `config/` — `config.example.h` (safe template)
- `docs/` — Audits, power design, architecture, build guide, test report, cleanup report, `images/`
- `firmware/` — `shared/include/`, `esp32cam/`, `nano/`
- `hardware/` — `BOM.md`, `GPIO_MAPPING.md`, `block_diagram/`, `power_distribution/`, `wiring_diagram/`
- `software/` — `web_dashboard/` reference UI & `telegram/` setup guide
- `source_material/` — Original `PROJECT_BRIEF.md`
- `tests/` — `unit/`, `integration/`, `test_plan.md`, `test_results/`

---

## 17. Development Roadmap
- **Completed:** Phase 1 Hardware/GPIO/Power Audit, Option A Firmware (`esp32cam` + `nanoatmega328`), Web Dashboard, Telegram Integration, Automated Tests, Repository Cleanup, `PROJECT_MASTER.md` & `AGENTS.md`, GitHub Publication, and User Physical Hardware Verification (`2026-11-27`).
- **Current:** Project 100% complete and maintained.
- **Future Improvements:** See Section 11 (3S LiPo + BMS pack, captive-portal NVS provisioning, `TB6612FNG` MOSFET driver).

---

## 18. Change History

| Date | Files Changed | Summary of Change | Reason | Build / Test Impact | Docs Updated |
| :--- | :--- | :--- | :--- | :--- | :--- |
| 2026-11-27 | `docs/HARDWARE_AUDIT.md`, `GPIO_FEASIBILITY.md`, `POWER_DESIGN.md`, `KNOWN_UNKNOWNS.md`, `hardware/BOM.md` | Completed Phase 1 audit, selected Option A (`74AHCT125` + `1k/2k` divider + Buck `5.0V` + `3A` provisional fuse) | Resolve ESP32-CAM GPIO shortage & safety risks | PlatformIO `6.2.0` verified | Phase 1 docs |
| 2026-11-27 | `platformio.ini`, `config/`, `firmware/*`, `hardware/*`, `software/*`, `tests/*`, `README.md` | Generated complete dual-MCU firmware, dashboard, Telegram notifier, diagrams, and test suite | Re-implement lost prototype cleanly | `esp32cam` & `nanoatmega328` PASS; `10/10` tests PASS | `ARCHITECTURE.md`, `BUILD_GUIDE.md`, `TEST_REPORT.md` |
| 2026-11-27 | `PROJECT_MASTER.md`, `AGENTS.md`, `docs/CLEANUP_REPORT.md`, removed `.pio*`/`__pycache__` | Removed `1.83 GB` generated caches; created single-source-of-truth master doc & agent maintenance rules | Optimize repo size & enforce doc sync | Clean re-test `10/10 PASS` | `PROJECT_MASTER.md`, `AGENTS.md`, `CLEANUP_REPORT.md` |
| 2026-11-27 | `PROJECT_MASTER.md`, `README.md`, `hardware/GPIO_MAPPING.md`, `docs/TEST_REPORT.md`, `tests/test_results/latest_results.json` | Recorded user-confirmed physical hardware verification (`2026-11-27`) and finalized 100% completion status | User completed & approved hardware tests | `10/10` automated tests PASS; hardware user-verified | `PROJECT_MASTER.md`, `README.md`, `TEST_REPORT.md`, `GPIO_MAPPING.md` |
| 2026-11-27 | `README.md`, `PROJECT_MASTER.md` | Added YouTube demo video link (`https://www.youtube.com/watch?v=9W-vfZcfwpw`) at top of `README.md` and in `PROJECT_MASTER.md` | Highlight live hardware demo first in repo | `10/10` automated tests PASS | `README.md`, `PROJECT_MASTER.md` |
