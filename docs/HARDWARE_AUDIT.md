# Hardware Feasibility Audit — ESP32-CAM Autonomous Surveillance Robot

> **Audit Status:** PHASE 1 (Pre-Implementation Hardware & Architecture Review)  
> **Safety Notice:** Do **NOT** power on or wire the final hardware until the selected architecture, GPIO assignments, and buck converter voltage output (`5.0V`) are reviewed and verified.

---

## 1. Supplied Project Material Inspection

The following project inputs were inspected prior to any firmware generation:

1. [`source_material/PROJECT_BRIEF.md`](../source_material/PROJECT_BRIEF.md)
   - Controller: **AI-Thinker ESP32-CAM**
   - Camera: **OV3660**
   - Sensors: **PIR** (human motion detection), **HC-SR04** (ultrasonic distance measurement)
   - Motor subsystem: **L298N** dual H-bridge motor driver, **2 single-shaft BO motors (rated 6–12V)**
   - Alerting: **Small 5V buzzer**
   - Power parts: **12V, 3A adapter**, **buck converter**, **capacitors and resistors**
   - Communication: **Wi-Fi**, browser web interface, **Telegram Bot API**
   - Missing history explicitly noted: original firmware unavailable, GPIO connections unknown, wiring diagram unavailable, original test results unavailable.

2. [`docs/images/esp32-cam-robot-project.jpg`](images/esp32-cam-robot-project.jpg)
   - **Visually observed hardware:**
     - Double-deck yellow robot chassis with four visible yellow wheels (see [KNOWN_UNKNOWNS.md](KNOWN_UNKNOWNS.md) regarding 2-motor vs. 4-wheel layout).
     - Front-mounted **PIR motion sensor** (white Fresnel dome on front-left), **AI-Thinker ESP32-CAM** module mounted upright in the center-front, and **HC-SR04** ultrasonic transducer pair on front-center-right.
     - A black wall-plug **12V adapter** resting on the upper deck with its power cable routed into the lower deck.
     - Internal wiring, a yellow-geared BO motor, and small green PCB/regulator hardware partially visible between the decks.
   - **Visually observed UI reference (bottom-right inset):**
     - Split-panel layout showing **LIVE STREAM** (left) and **CAPTURED IMAGE** (right) with an overlaid distance label (`Distance: 45 cm`), plus five directional control buttons (`Forward`, `Left`, `Stop`, `Right`, `Back`).

---

## 2. Controller Audit: AI-Thinker ESP32-CAM

### 2.1 Core Hardware Specifications
- **SoC / Module:** ESP32-S (ESP32-D0WD dual-core Tensilica LX6 up to 240 MHz, Wi-Fi 802.11 b/g/n + Bluetooth).
- **Memory:** Typically 4 MB QIO SPI Flash + 4 MB external PSRAM (PSRAM CS on `GPIO 16`; exact board revision must be confirmed at boot via `psramFound()`).
- **USB-UART Interface:** **None onboard.** Programming and serial monitoring use the supplied **ESP32-CAM-MB** carrier board (CH340C USB-to-UART bridge wired to `GPIO 1` / `GPIO 3`, DTR/RTS auto-reset into `EN` and `GPIO 0`, plus `IO0` and `RST` pushbuttons).
- **Onboard Peripherals Consuming GPIOs:**
  - **24-pin 0.5 mm FPC Camera Connector:** Occupies 14 dedicated internal/header GPIOs (`GPIO 0, 5, 18, 19, 21, 22, 23, 25, 26, 27, 32, 34, 35, 36, 39` — 15 pins total including `PWDN`).
  - **PSRAM:** Uses `GPIO 16` (`CS`) and internal flash/PSRAM SPI bus (`GPIO 17` clock). `GPIO 16` **cannot** be used as external GPIO while PSRAM is active.
  - **MicroSD Card Slot (TF):** Connected to `GPIO 2, 4, 12, 13, 14, 15` (4-bit SD_MMC mode), or `GPIO 2, 14, 15` (1-bit SD_MMC mode).
  - **Onboard White Flash LED:** Driven via an NPN transistor from `GPIO 4` (shared with SD `DATA1`). Draws ~100–300 mA when illuminated.
  - **Onboard Red Status LED:** Active-LOW on `GPIO 33` (not broken out to headers; safe for status indication).

### 2.2 Boot-Strapping & Flash Risk Summary
- **`GPIO 0`:** Camera `XCLK` output (20 MHz) AND boot-mode strapping pin (`HIGH` = normal SPI boot, `LOW` = UART download mode). **Unavailable for robot peripherals.**
- **`GPIO 12` (MTDI):** Flash voltage strapping pin. **Must be LOW at power-up/reset.** If an external pull-up or sensor drives `GPIO 12` HIGH during boot, the internal LDO selects 1.8V instead of 3.3V and the ESP32 fails to boot.
- **`GPIO 2`:** Must not be pulled HIGH while `GPIO 0` is LOW during serial flashing.
- **`GPIO 15` (MTDO):** Has an internal pull-up; controls ROM boot log verbosity (`LOW` silences boot ROM prints).
- **`GPIO 1` (`U0TXD`) & `GPIO 3` (`U0RXD`):** Dedicated to `ESP32-CAM-MB` flashing and serial debug console. Also, `GPIO 1` outputs 115200 bps boot diagnostics on every reset.

---

## 3. Camera Audit: OV3660 Sensor & Software Support

### 3.1 Electrical & Mechanical Compatibility
- **Interface:** 24-pin 0.5 mm pitch DVP parallel camera ribbon + SCCB (I2C-like) control bus (`SIOD = GPIO 26`, `SIOC = GPIO 27`).
- **Drop-In Compatibility:** Standard AI-Thinker ESP32-CAM boards ship with an OV2640, and 24-pin **OV3660** modules share the compatible pinout on the AI-Thinker FPC footprint.
- **FPC Orientation Caution:** The gold contact fingers on the 24-pin FPC ribbon must match the bottom-contact pins of the AI-Thinker FPC latch and be seated squarely before closing the locking tab. Misalignment causes `0x105` / `ESP_ERR_CAMERA_NOT_FOUND` or short circuits.

### 3.2 Software Driver & Exact Version Compatibility
| Software Component | Pinned Target Version | OV3660 Support Status | Notes |
| :--- | :--- | :--- | :--- |
| PlatformIO Platform | `espressif32 @ 6.9.0` (or `6.8.1`–`6.10.0`) | **Supported** | Bundles `arduino-esp32` **v2.0.17** (proven stable baseline for `esp32cam`). |
| Camera Driver | `esp32-camera` (bundled in `arduino-esp32` `2.0.17`) | **Supported** (`ov3660.c`, PID `0x3660`) | Auto-detected by `esp_camera_init(&config)` over SCCB using AI-Thinker pin definitions. |
| Alternative (Arduino 3.x) | `pioarduino/platform-espressif32` | Supported | Optional newer IDF 5.x core; `espressif32 @ 6.9.0` is more predictable for legacy libraries unless v3.x is specifically needed. |

### 3.3 OV3660 Operational Caveats
1. **Mandatory PSRAM:** `BOARD_HAS_PSRAM` and `-mfix-esp32-psram-cache-issue` build flags must be enabled. Without PSRAM, frame buffers are severely restricted (`QVGA`, single buffer) and streaming stalls.
2. **Higher Power & Heat than OV2640:** The 3MP OV3660 sensor draws higher active current and runs noticeably warmer during continuous streaming. Recommend **VGA (`640x480`)** or **SVGA (`800x600`)** with `PIXFORMAT_JPEG` and `fb_count = 2` (`CAMERA_GRAB_LATEST`) for responsive MJPEG streaming without overheating or Wi-Fi starvation.
3. **Image Orientation:** OV3660 modules commonly mount inverted relative to OV2640; software `s->set_vflip(s, 1)` / `s->set_hmirror(s, 1)` adjustment should be configurable.

---

## 4. Peripheral & Actuator Audit

| Component | Required Signals | Logic Level | Key Engineering Constraint |
| :--- | :--- | :--- | :--- |
| **PIR Motion Sensor** | 1 Digital Input (`OUT`) | Typically 3.3V output (HC-SR501) from 5V `VCC` | Must verify output voltage is ≤3.3V before connecting directly to ESP32; add 1kΩ protection or divider if 5V output variant. |
| **HC-SR04 Ultrasonic** | 1 Output (`TRIG`), 1 Input (`ECHO`) | 5V `VCC`, **5V `ECHO` output** | `ECHO` **must** pass through a 5V→3.3V voltage divider if wired to ESP32. Safe at 5V directly if wired to Arduino Uno/Nano. |
| **L298N Motor Driver** | 4 Direction (`IN1`–`IN4`) + optional 2 PWM (`ENA`, `ENB`) | TTL/CMOS (`VIH ≥ 2.3V`, compatible with 3.3V and 5V) | Drops ~2.0–3.2V across internal BJT H-bridge. Onboard 78M05 must **NOT** power ESP32-CAM. |
| **2x BO Motors (6–12V)** | Driven by L298N `OUT1–4` | ~9–10V effective from 12V via L298N | High startup/stall current (1–2.5A) causes 12V noise & dip; requires bulk + ceramic decoupling. |
| **Small 5V Buzzer** | 1 Output (`CTRL`) | 5V coil/piezo (15–30 mA typical) | Do **not** drive directly from a 3.3V ESP32 GPIO; use an NPN transistor driver (e.g., 2N2222/BC547/S8050 + 1kΩ base resistor + flyback diode if magnetic) powered from 5V. |

---

## 5. GPIO Shortage Verdict & Candidate Architectures

**Minimum external I/O signals required:** `1 (PIR) + 2 (HC-SR04) + 4 (L298N IN1–4) + 1 (Buzzer) = 8 GPIOs`  
*(10 GPIOs if `ENA`/`ENB` PWM speed control is included).*

**Cleanly available GPIOs on AI-Thinker ESP32-CAM (with Camera + PSRAM + UART0 preserved + Flash LED preserved + SD disabled):**  
Only **5 pins** (`GPIO 12, 13, 14, 15, 2`) — and `GPIO 12`, `GPIO 2`, `GPIO 15` carry boot-strapping rules. Even sacrificing `GPIO 4` (Flash LED) gives only **6 pins**. Reaching 8 pins directly on the ESP32-CAM forces sacrificing **UART0 (`GPIO 1` & `GPIO 3`)** as well.

Detailed pin-by-pin analysis is documented in [`docs/GPIO_FEASIBILITY.md`](GPIO_FEASIBILITY.md).

### Architecture Comparison

#### Option A: ESP32-CAM + Arduino Uno/Nano Co-Processor *(RECOMMENDED)*
- **Concept:** ESP32-CAM handles OV3660 camera, MJPEG streaming, web dashboard, Wi-Fi, image storage, and Telegram HTTPS uploads. An **Arduino Nano or Uno** (hardware you already have) handles real-time ultrasonic ranging, emergency obstacle stop, L298N motor direction + PWM speed control (`ENA`/`ENB`), PIR monitoring, and 5V buzzer driving, communicating with ESP32-CAM over UART (`Serial2` mapped to `GPIO 13` / `GPIO 14` on ESP32-CAM).
- **Advantages:**
  - **Zero new ICs required** (uses your available Arduino Uno/Nano + resistors).
  - Keeps **UART0 (`GPIO 1`/`GPIO 3`) 100% free** on ESP32-CAM for `ESP32-CAM-MB` flashing and USB serial debugging without disconnecting robot wires.
  - Keeps **Flash LED (`GPIO 4`)** available (and even **1-bit MicroSD** remains possible on `GPIO 2/14/15` if UART2 uses `GPIO 12/13`).
  - **Native 5V I/O on Arduino:** HC-SR04 `ECHO`, PIR, L298N (`IN1–4` + `ENA`/`ENB` PWM), and 5V buzzer operate at 5V cleanly. Only a 2-resistor 5V→3.3V divider is needed on `Arduino TX → ESP32 RX`.
  - **Deterministic safety:** Emergency obstacle stop executes on the Arduino in <30 ms even while ESP32-CAM is busy with Wi-Fi, JPEG compression, or TLS Telegram uploads.
- **Limitations:**
  - Two firmware targets (`esp32cam` and `arduino_nano`/`uno`) maintained in the same PlatformIO project.
  - Requires mounting space and 5V power feed for the Arduino board.
- **Additional Components:** Arduino Uno/Nano (already owned), 2 resistors for `Arduino TX → ESP32 RX` divider (e.g., `1kΩ + 2kΩ`, from resistor kit).

#### Option B: ESP32-CAM + I2C GPIO Expander (`PCF8574` or `MCP23017`)
- **Concept:** ESP32-CAM uses 2 pins (`GPIO 14` SDA, `GPIO 15` SCL at 3.3V) to control L298N `IN1–IN4` and the buzzer transistor via a `PCF8574` (8-bit) or `MCP23017` (16-bit) I2C expander, while keeping `PIR`, `TRIG`, and `ECHO` directly on ESP32-CAM GPIOs (`13`, `2`, `12` or similar).
- **Advantages:** Single firmware binary; preserves `UART0` (`GPIO 1/3`) and `GPIO 4` (Flash LED).
- **Limitations:** **Requires purchasing an external IC/breakout (`PCF8574`/`MCP23017`)** not listed in your hardware; no hardware PWM on `ENA`/`ENB`; all tasks share the ESP32-CAM CPU.
- **Additional Components:** `PCF8574` or `MCP23017` module, two 4.7kΩ pull-up resistors to 3.3V, 5V→3.3V divider on `ECHO`, NPN transistor driver for buzzer.

#### Option C: Direct Single-Controller ESP32-CAM Only *(NOT RECOMMENDED unless zero extra MCU is strict)*
- **Concept:** Wire all 8 signals directly to `GPIO 12, 13, 14, 15, 2, 4, 1, 3`.
- **Advantages:** Single firmware, zero extra controller hardware.
- **Limitations (Severe):**
  - Must **disable MicroSD** permanently.
  - Must repurpose **`GPIO 4` (onboard 1W Flash LED)** — any HIGH output lights the Flash LED (~100–300 mA brownout hazard) unless mapped only to a microsecond pulse (e.g. `TRIG`) or hardware-modified.
  - Must repurpose **both `GPIO 1` (TX) and `GPIO 3` (RX)** — **eliminates serial debugging**, causes 115200 bps boot chatter on `GPIO 1` during reset, and forces **disconnecting wires** before using `ESP32-CAM-MB` to flash firmware.
  - No pins available for L298N `ENA`/`ENB` PWM speed control.
  - Tight boot-strapping constraints on `GPIO 12` and `GPIO 2`.
- **Additional Components:** 5V→3.3V divider on `ECHO`, NPN transistor + resistors for 5V buzzer, jumper/disconnect headers for `GPIO 1/3` during flashing.

---

## 6. Phase 1 Decision Gate & Option A Verification

**Selected Architecture (User Approved):** **Option A — AI-Thinker ESP32-CAM + Arduino Nano (ATmega328P, 5V/16 MHz) over UART**

### 6.1 Chassis & Caster Wheel Verification
- The brief specifies **2 single-shaft BO motors (6–12V)**, while [`docs/images/esp32-cam-robot-project.jpg`](images/esp32-cam-robot-project.jpg) visually depicts a 4-wheel double-deck chassis.
- **Mechanical Requirement:** A 2-motor differential-drive robot has only two driven contact points and **will tip forward or backward** onto the lower deck or front sensors unless at least **one additional passive support point** is present.
  - **If your physical chassis has 2 driven wheels + 2 passive/free-spinning wheels** (matching the 4-wheel photo), no extra caster is needed—though verify the 2 unpowered wheels spin freely without dragging during turns.
  - **If your physical chassis only has the 2 driven BO motor wheels**, **at least 1 swivel/ball caster wheel IS required** (mounted at the end opposite the driven axle, or 2 casters if mid-mounted) to stabilize the chassis and keep the HC-SR04/PIR clear of the floor.

### 6.2 Development Toolchain Check (`PlatformIO`)
- Checked local environment:
  - `pio` / `platformio`: **Not installed** on `PATH`.
  - `Python`: `3.14.7` (`pip 26.3`), `Node`: `v26.5.1`, `uv`: installed at `C:\Users\venka\.local\bin\uv.exe`.
  - **Compatibility Finding:** PlatformIO Core supports **Python 3.10–3.13** and can fail on Python 3.14. Because `uv` is available, we can create an isolated Python `3.12`/`3.13` virtual environment (`.pio-venv/`) inside the workspace (excluded in `.gitignore`) or in user space and install PlatformIO cleanly once you approve Phase 2 / dependency installation.

### 6.3 Remaining Approval Gate Before Phase 2
Before generating final hardware-dependent firmware, please review and approve:
1. The finalized **PROPOSED** Option A UART & peripheral wiring table in [`docs/GPIO_FEASIBILITY.md`](GPIO_FEASIBILITY.md#L64-L116).
2. The bidirectional UART voltage-level conditioning (`1kΩ + 2kΩ` divider on `Nano D9 TX → ESP32 GPIO 13 RX`, and `470Ω–1kΩ` series resistor + disabled `INPUT_PULLUP` on `ESP32 GPIO 14 TX → Nano D8 RX`).
3. The power distribution diagram and Buck Converter `5.0V` setup in [`docs/POWER_DESIGN.md`](POWER_DESIGN.md).
4. Permission to install **PlatformIO** in an isolated Python `3.12` virtual environment via `uv` so we can compile both `esp32cam` and `nanoatmega328` firmware targets in Phase 2/3.
