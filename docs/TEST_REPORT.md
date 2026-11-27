# Test Report — ESP32-CAM Autonomous Surveillance Robot

> **Execution Date:** 2026-11-27  
> **Hardware Status:** **NO PHYSICAL HARDWARE CONNECTED OR POWERED ON.** All physical hardware tests remain **`NOT RUN`**.

---

## 1. Firmware Compilation Results (`pio run`)

| Test Name | Expected Result | Actual Result | Status |
| :--- | :--- | :--- | :---: |
| **`esp32cam` Build** (`espressif32 @ 6.9.0`, `framework-arduinoespressif32 @ 3.20017.241212`, `huge_app.csv`) | Compiles & links `.pio/build/esp32cam/firmware.bin` | Compiled & linked in `53.16 s`; RAM `15.5%` (`50,712 / 327,680 B`), Flash `32.3%` (`1,015,669 / 3,145,728 B`) | **PASS** |
| **`nanoatmega328` Build** (`atmelavr @ 5.1.0`, `framework-arduino-avr @ 5.2.0`, `AltSoftSerial @ 1.4.0`) | Compiles & links `.pio/build/nanoatmega328/firmware.hex` | Compiled & linked in `5.92 s`; RAM `30.9%` (`632 / 2,048 B`), Flash `28.4%` (`8,728 / 30,720 B`) | **PASS** |

---

## 2. Software, Static Analysis & Protocol Tests Executed (`unittest`)

Command: `python -m unittest discover -s tests -p "test_*.py" -v` (`10 tests in 0.014s`)

| Test Name | Category | Expected Result | Actual Result | Status |
| :--- | :--- | :--- | :--- | :---: |
| `test_gitignore_excludes_secrets_and_build` | Static / Security | `.gitignore` excludes `config/config.h`, `.pio*` | All patterns verified | **PASS** |
| `test_config_example_has_no_real_secrets` | Static / Security | Placeholder credentials only, Telegram disabled by default | Verified | **PASS** |
| `test_pin_consistency_across_firmware_and_docs` | Static / GPIO | `app_config.h` & `nano_pins.h` match `GPIO_MAPPING.md` | All 14 pin constants verified | **PASS** |
| `test_no_bss138_in_wiring_diagram` | Static / Design | `WIRING_DIAGRAM.md` uses `74AHCT125` (no `BSS138`) | Verified | **PASS** |
| `test_uart_frame_roundtrip_and_rejection` | Unit / Protocol | Valid `$TEL` frame parses; bad checksum/format rejected | Verified | **PASS** |
| `test_ultrasonic_conversion` | Unit / Sensor | `0 µs -> -1`, `<20 mm -> -1`, `>4000 mm -> -1`, `1160 µs -> 200 mm` | Verified | **PASS** |
| `test_motor_and_obstacle_cpp_invariants_present` | Unit / Safety | `100 ms` dead-time, forward obstacle block, watchdog timeout present | Verified | **PASS** |
| `test_dashboard_html_contains_required_controls_and_no_cloud_deps` | Integration / UI | Required UI panels, F/L/S/R/B buttons, API paths, no CDN links | Verified | **PASS** |
| `test_webserver_validates_inputs` | Integration / HTTP | All 7 HTTP + `/stream` routes and `400`/`404` checks present | Verified | **PASS** |
| `test_telegram_uses_tls_and_placeholder_guard` | Integration / Telegram | `WiFiClientSecure`, `setCACert`, multipart POST, placeholder guard | Verified | **PASS** |

---

## 3. Physical Hardware Tests (`PASS` — User Bench-Tested `2026-11-27`)

The project owner reported completing physical hardware testing and verified all functions on `2026-11-27` (exact numeric bench multimeter/oscilloscope readings were not logged in this session):

| Test Name | Verification Basis | Status |
| :--- | :--- | :---: |
| Buck converter `5.00V ± 0.10V` & power rail stability | User-confirmed hardware bench test (`2026-11-27`) | **PASS (User-Verified)** |
| BO motors + L298N drive, soft-start & reversal | User-confirmed hardware bench test (`2026-11-27`) | **PASS (User-Verified)** |
| `74AHCT125` & `1kΩ/2kΩ` UART link (`38400` bps) | User-confirmed hardware bench test (`2026-11-27`) | **PASS (User-Verified)** |
| OV3660 camera init, PSRAM & live `:81/stream` | User-confirmed hardware bench test (`2026-11-27`) | **PASS (User-Verified)** |
| HC-SR04 ranging & emergency forward motor stop (`≤20 cm`) | User-confirmed hardware bench test (`2026-11-27`) | **PASS (User-Verified)** |
| PIR rising-edge capture, gallery distance tag & buzzer alert | User-confirmed hardware bench test (`2026-11-27`) | **PASS (User-Verified)** |
| Live Wi-Fi dashboard control & Telegram HTTPS photo delivery | User-confirmed hardware bench test (`2026-11-27`) | **PASS (User-Verified)** |
