# GPIO Feasibility & Proposed Pin Mapping — AI-Thinker ESP32-CAM

> **Status:** PHASE 1 AUDIT — **ALL PROPOSED PIN ASSIGNMENTS BELOW ARE `PROPOSED`, NOT `VERIFIED`.**  
> Do not wire hardware until the target architecture is approved and physical continuity/voltage levels are checked.

---

## 1. AI-Thinker ESP32-CAM Complete Pin Audit

### 1.1 Internal Camera & Memory Allocations (Non-Negotiable)

| ESP32 GPIO | Internal Assignment | Header Exposed? | Available for Robot I/O? | Reason / Constraint |
| :--- | :--- | :--- | :--- | :--- |
| **GPIO 0** | Camera `XCLK` (20 MHz) + Boot Strapping | Yes (P1) | **NO** | Must be HIGH for normal boot, LOW for flash mode, and carries 20 MHz camera clock. |
| **GPIO 5** | Camera `Y2` (D0) | No | **NO** | Hardwired to camera FPC. |
| **GPIO 18** | Camera `Y3` (D1) | No | **NO** | Hardwired to camera FPC. |
| **GPIO 19** | Camera `Y4` (D2) | No | **NO** | Hardwired to camera FPC. |
| **GPIO 21** | Camera `Y5` (D3) | No | **NO** | Hardwired to camera FPC. |
| **GPIO 22** | Camera `PCLK` | No | **NO** | Hardwired to camera FPC. |
| **GPIO 23** | Camera `HREF` | No | **NO** | Hardwired to camera FPC. |
| **GPIO 25** | Camera `VSYNC` | No | **NO** | Hardwired to camera FPC. |
| **GPIO 26** | Camera `SIOD` (SCCB SDA) | No | **NO** | Hardwired to camera FPC. |
| **GPIO 27** | Camera `SIOC` (SCCB SCL) | No | **NO** | Hardwired to camera FPC. |
| **GPIO 32** | Camera `PWDN` (active HIGH power-down) | No | **NO** | Hardwired to camera power-down switch. |
| **GPIO 34** | Camera `Y8` (D6, input-only) | No | **NO** | Hardwired to camera FPC. |
| **GPIO 35** | Camera `Y9` (D7, input-only) | No | **NO** | Hardwired to camera FPC. |
| **GPIO 36** | Camera `Y6` (D4, input-only) | No | **NO** | Hardwired to camera FPC. |
| **GPIO 39** | Camera `Y7` (D5, input-only) | No | **NO** | Hardwired to camera FPC. |
| **GPIO 16** | **PSRAM CS** (`U2RXD` silkscreen on some boards) | Yes (P1) | **NO** | Required for OV3660 PSRAM frame buffers. Toggling causes PSRAM corruption/Guru Meditation crash. |
| **GPIO 33** | Onboard Red Status LED (Active LOW) | No | Internal LED only | Safe for firmware heartbeat/status LED. |

### 1.2 Exposed Header Pins (`P1` & `P2`, 16 Pins Total)

| Header Pin | Function / Alias | Shared Onboard Peripheral | Strapping / Boot Rule | Usability Verdict |
| :--- | :--- | :--- | :--- | :--- |
| **5V** | 5V Power Input | Power rail | N/A | **Power IN** (from regulated 5.0V Buck Converter). Never feed 12V! |
| **3V3** | 3.3V Output (AMS1117) | Power rail | N/A | Reference / light pull-ups only (not for motors/5V sensors). |
| **GND** (x2/x3) | Ground | Power rail | N/A | Common star ground. |
| **GPIO 12** | `HS2_DATA2` / `MTDI` | MicroSD (4-bit mode) | **CRITICAL:** Must be **LOW** at boot/reset (selects 3.3V flash). | **Usable if SD disabled** AND external circuit never pulls HIGH at boot. |
| **GPIO 13** | `HS2_DATA3` | MicroSD (4-bit & 1-bit) | None | **Cleanest free GPIO** (when SD card is unused). |
| **GPIO 14** | `HS2_CLK` / `MTMS` | MicroSD (4-bit & 1-bit) | Internal pull-up | **Clean free GPIO** (when SD card is unused). |
| **GPIO 15** | `HS2_CMD` / `MTDO` | MicroSD (4-bit & 1-bit) | Internal pull-up; LOW silences ROM log | **Free GPIO** (when SD card is unused). |
| **GPIO 2** | `HS2_DATA0` | MicroSD (4-bit & 1-bit) | Must NOT be pulled HIGH during serial flash (`GPIO0=LOW`) | **Usable if SD disabled** and idle LOW/floating during flash. |
| **GPIO 4** | `HS2_DATA1` + **Flash LED** | MicroSD (4-bit) + **1W White LED (Q1)** | None | **Conflict:** Driving HIGH turns on bright Flash LED (~100–300 mA). Keep for Flash LED or microsecond-only pulse. |
| **GPIO 1** | `U0TXD` | `ESP32-CAM-MB` CH340C RX | Outputs 115200 bps ROM boot log on reset | **Avoid repurposing:** Breaks Serial logging & `ESP32-CAM-MB` flashing unless disconnected. |
| **GPIO 3** | `U0RXD` | `ESP32-CAM-MB` CH340C TX | Held HIGH by CH340C when MB board attached | **Avoid repurposing:** Breaks Serial RX & `ESP32-CAM-MB` flashing unless disconnected. |
| **GPIO 0** | `XCLK` / Boot | Camera + `ESP32-CAM-MB` | Must be HIGH to run, LOW to flash | **Reserved (Camera + Boot).** |
| **GPIO 16** | `PSRAM` | Onboard PSRAM | N/A | **Reserved (PSRAM).** |

---

## 2. Robot Signal Budget vs. Available ESP32-CAM Pins

| Signal | Direction | Count | Critical Timing / Electrical Notes |
| :--- | :--- | :---: | :--- |
| `L298N IN1–IN4` | Output | 4 | Direction control (`3.3V` exceeds L298N `2.3V` `VIH` threshold). |
| `L298N ENA, ENB` | Output (PWM) | 0 or 2 | Optional for speed control / soft-start; 0 pins if jumpers stay installed. |
| `HC-SR04 TRIG` | Output | 1 | 10 µs trigger pulse. |
| `HC-SR04 ECHO` | Input | 1 | Microsecond pulse-width read (`5V` sensor output requires divider for ESP32). |
| `PIR OUT` | Input | 1 | Digital motion state / edge interrupt. |
| `BUZZER CTRL` | Output | 1 | Drives NPN transistor for 5V buzzer. |
| **Total Required** | | **8 (min) / 10 (with PWM)** | |
| **Clean ESP32-CAM Pins** | (SD off, UART0 kept, Flash kept) | **5** (`13, 14, 15, 12, 2`) | **Insufficient by 3–5 pins** without sacrificing UART0 + Flash LED. |

**Conclusion:** Direct single-board wiring cannot preserve UART0 debugging/flashing, Flash LED safety, and all 8 robot signals simultaneously. Adding either the **user's Arduino Uno/Nano (Option A)** or an **I2C GPIO Expander (Option B)** resolves all GPIO conflicts cleanly.

---

## 3. Proposed Connection Tables (Marked `PROPOSED`)

### 3.1 Option A (Architecture Conditionally Approved; All Connections `PROPOSED`): ESP32-CAM + Arduino Nano (5V / 16 MHz)

#### 3.1.1 Finalized UART Pin Selection, Camera/Boot & `AltSoftSerial` Timer1 Audit (`PROPOSED`)
ESP32-CAM uses hardware `Serial2` (`HardwareSerial(2)`) remapped directly via `Serial2.begin(38400, SERIAL_8N1, 13, 14)`. Arduino Nano uses `AltSoftSerial` (`D8` RX / `D9` TX on Timer1).

| Side | Selected Pin | Native Signal / Alias | Conflict Check (Camera, Boot, PSRAM, Timers) | Status |
| :--- | :--- | :--- | :--- | :--- |
| **ESP32-CAM** | `GPIO 14` | `UART2_TX` (to level buffer → Nano `D8`) | **CLEAR:** Not a camera pin, not PSRAM (`GPIO 16`), not boot-mode strapping (`GPIO 0/2/12/15`), not Flash LED (`GPIO 4`), not `UART0` (`GPIO 1/3`). Briefly has internal JTAG `MTMS` pull-up at reset before `Serial2.begin()`, harmless to buffer input. | **PROPOSED** |
| **ESP32-CAM** | `GPIO 13` | `UART2_RX` (from Nano `D9` via divider) | **CLEAR:** Not a camera pin, not PSRAM, **not a strapping pin** (`MTCK` has no boot-mode or flash-voltage strapping role), not Flash LED, not `UART0`. | **PROPOSED** |
| **Arduino Nano** | `D8` | `AltSoftSerial RX` (`PB0` / `ICP1`) | **CLEAR:** Leaves `D0/D1` (USB Serial) free for Nano flashing/debug. `AltSoftSerial` enables `INPUT_PULLUP` on `D8`; driving through an active `74AHCT125` (or `BSS138` stage) avoids any 5V back-feed to ESP32 `GPIO 14`. | **PROPOSED** |
| **Arduino Nano** | `D9` | `AltSoftSerial TX` (`PB1` / `OC1A`) | **CLEAR:** `AltSoftSerial` takes exclusive ownership of **Timer1** (`D9`/`D10` lose `analogWrite()` PWM, and `Servo.h` cannot be used). `D10` remains fully usable as digital `TRIG` output. **Timer0 (`D5`/`D6` PWM for `ENA`/`ENB`)** and **Timer2 (`tone()` for buzzer)** are completely unaffected. | **PROPOSED** |

*(Note: `GPIO 1` & `GPIO 3` remain free for `ESP32-CAM-MB`; `GPIO 4` remains free for optional Flash LED; `GPIO 33` remains free for onboard Red status LED; `GPIO 12`, `GPIO 2`, `GPIO 15` remain untouched/free; MicroSD slot is left unused so `GPIO 13/14` have zero SD bus conflict.)*

#### 3.1.2 Bidirectional UART Voltage-Level Compatibility Check
| Link Direction | Transmitter Output | Receiver Input Threshold | Conditioning Circuit | Status |
| :--- | :--- | :--- | :--- | :--- |
| **ESP32 `GPIO 14` (3.3V TX) → Nano `D8` (5V RX)** | `~3.25–3.30 V` HIGH | ATmega328P `VIH(min) = 0.6 × VCC` (`3.00 V` at `5.00 V`) | **PROPER LEVEL BUFFER REQUIRED (do not connect directly).** Use a **`74AHCT125` (or `74HCT125` / `74AHCT1G125`) 5V-powered TTL-input buffer** (`VCC = 5.0V`, `VIH = 2.0V` min, unused `/OE` tied to `GND`, unused inputs tied to `GND`). Translates `3.3V` cleanly to full **`5.0V` push-pull** (`~2.0 V` noise margin). *(Alternative: single channel of a `BSS138` 3.3V↔5V module with `LV=3.3V`, `HV=5.0V`.)* | **PROPOSED** |
| **Nano `D9` (5V TX) → ESP32 `GPIO 13` (3.3V RX)** | `~4.2–5.0 V` HIGH | ESP32 `VIH ≥ 2.48 V`, **Abs Max `3.6 V`** | **MANDATORY RESISTOR VOLTAGE DIVIDER (retained).** `Nano D9` → `R1 = 1 kΩ` → `ESP32 GPIO 13` → `R2 = 2 kΩ` (or `1.8 kΩ`) → `GND`. Output = **`3.33 V`** (or `3.21 V`), safe for 3.3V ESP32 input (`τ ≈ 10 ns`, supports `38400` bps cleanly). | **PROPOSED** |

#### 3.1.3 Complete Option A Proposed Wiring Table (`PROPOSED`)
| # | From Component & Pin | Level / Conditioning | To Component & Pin | Purpose / Notes | Status |
| :---: | :--- | :--- | :--- | :--- | :--- |
| **1** | ESP32-CAM `GPIO 14` (`UART2_TX`) | `3.3V` → **`74AHCT125` (or `BSS138`) `3.3V→5.0V` buffer** | Arduino Nano `D8` (`RX`) | UART TX (`38400` bps) with full `5.0V` logic margin | **PROPOSED** |
| **2** | Arduino Nano `D9` (`TX`) | `5.0V` → **`1 kΩ` series + `2 kΩ` to `GND` divider (`3.33V`)** | ESP32-CAM `GPIO 13` (`UART2_RX`) | UART RX (`38400` bps) stepped down safely to `3.3V` | **PROPOSED** |
| **3** | `5.0V` Buck Rail (and ESP32 `3V3` if `BSS138` used) | `VCC = 5.0V` (`74AHCT125`), `/OE1 = GND`, unused inputs `GND` | `74AHCT125` (or `BSS138` `HV=5V, LV=3V3`) | Power for UART 3.3V→5V level buffer | **PROPOSED** |
| **4** | Arduino Nano `D5` (Timer0 PWM) | `5V` (`10kΩ` pull-down to `GND` recommended) | L298N `ENA` | Left motor PWM speed (remove `ENA` jumper) | **PROPOSED** |
| **5** | Arduino Nano `D6` (Timer0 PWM) | `5V` (`10kΩ` pull-down to `GND` recommended) | L298N `ENB` | Right motor PWM speed (remove `ENB` jumper) | **PROPOSED** |
| **6** | Arduino Nano `D4` | `5V` logic | L298N `IN1` | Left motor direction A | **PROPOSED** |
| **7** | Arduino Nano `D7` | `5V` logic | L298N `IN2` | Left motor direction B | **PROPOSED** |
| **8** | Arduino Nano `D2` | `5V` logic | L298N `IN3` | Right motor direction A | **PROPOSED** |
| **9** | Arduino Nano `D3` | `5V` logic | L298N `IN4` | Right motor direction B | **PROPOSED** |
| **10** | Arduino Nano `D10` | `5V` digital output | HC-SR04 `TRIG` | 10 µs ultrasonic trigger (digital I/O; no PWM needed) | **PROPOSED** |
| **11** | HC-SR04 `ECHO` | `5V` native (direct to Nano) | Arduino Nano `D11` | Ultrasonic echo timing (`pulseIn` with timeout) | **PROPOSED** |
| **12** | PIR Sensor `OUT` | Verify PIR output (`5V` or `3.3V`; if `3.3V`, route via spare `74AHCT125` ch2) | Arduino Nano `D12` | Human motion detection (`D13` avoided due to Nano boot LED blink) | **PROPOSED** |
| **13** | Arduino Nano `A0` (`D14`) | `1kΩ` base resistor → NPN (`2N2222`/`BC547`/`S8050`) low-side switch (+ `1N4148` flyback diode) | 5V Buzzer `-` (`+` to `5V` rail) | Supports **both** active (`digitalWrite`) and passive (`tone()` via Timer2) buzzers safely | **PROPOSED** |
| **14** | L298N `OUT1`, `OUT2` | Unregulated H-bridge output (`100nF` ceramic cap across motor) | Left BO Motor | Single-shaft 6–12V motor | **PROPOSED** |
| **15** | L298N `OUT3`, `OUT4` | Unregulated H-bridge output (`100nF` ceramic cap across motor) | Right BO Motor | Single-shaft 6–12V motor | **PROPOSED** |
| **16** | Star `5.0V` Buck Rail | `5.00V ± 0.10V` (verified) + `470µF` + `100nF` local caps | ESP32-CAM `5V`, Nano `5V`, L298N `+5V` (**`5V-EN` jumper REMOVED**), PIR `VCC`, HC-SR04 `VCC`, Buzzer `+`, `74AHCT125 VCC` | **NEVER connect 12V here; unplug Nano `5V` when connecting Nano USB** | **PROPOSED** |
| **17** | Star `GND` Bus | Common Ground | Adapter `-`, Buck `IN-/OUT-`, L298N `GND`, ESP32-CAM `GND`, Nano `GND`, Sensors `GND`, `74AHCT125 GND` | Single star-ground reference | **PROPOSED** |

---

### 3.2 Option B (Alternative): ESP32-CAM + `PCF8574` I2C Expander

| ESP32-CAM Pin | Connected Component | Signal | Notes | Status |
| :--- | :--- | :--- | :--- | :--- |
| `GPIO 14` | `PCF8574 SDA` | I2C Data (3.3V) | 4.7kΩ pull-up to 3.3V | **PROPOSED** |
| `GPIO 15` | `PCF8574 SCL` | I2C Clock (3.3V) | 4.7kΩ pull-up to 3.3V | **PROPOSED** |
| `GPIO 13` | `PIR OUT` | Motion Input | Verify PIR outputs ≤3.3V | **PROPOSED** |
| `GPIO 12` | `HC-SR04 TRIG` | Ultrasonic Trigger | Output LOW at boot (safe for MTDI strapping) | **PROPOSED** |
| `GPIO 2` | `HC-SR04 ECHO` | Ultrasonic Echo | **MANDATORY 5V→3.3V divider** (`1kΩ + 2kΩ`); idle LOW during flashing | **PROPOSED** |
| `PCF8574 P0–P3` | `L298N IN1–IN4` | Motor Direction | `ENA`/`ENB` jumpers left ON (no PWM) | **PROPOSED** |
| `PCF8574 P4` | `5V Buzzer` (via NPN) | Alert Buzzer | Active HIGH drives transistor base | **PROPOSED** |
| `GPIO 1, GPIO 3` | `ESP32-CAM-MB` | UART0 Debug/Flash | Kept free | **PROPOSED** |
| `GPIO 4` | Onboard Flash LED | Illumination | Kept free | **PROPOSED** |

---

### 3.3 Option C (Fallback Only): Direct ESP32-CAM Only (Zero Extra Hardware)

> **Warning:** Requires disabling SD card, sacrificing UART0 serial debugging, disconnecting `GPIO 1/3` wires before `ESP32-CAM-MB` flashing, and mapping `GPIO 4` (Flash LED) strictly to a 10 µs ultrasonic trigger pulse (brief micro-flash only).

| ESP32-CAM Pin | Connected Component | Signal | Mandatory Risk Mitigation | Status |
| :--- | :--- | :--- | :--- | :--- |
| `GPIO 12` | `L298N IN1` | Left Forward | Must remain LOW at boot (verify L298N input does not pull HIGH) | **PROPOSED** |
| `GPIO 13` | `L298N IN2` | Left Reverse | Clean GPIO | **PROPOSED** |
| `GPIO 14` | `L298N IN3` | Right Forward | Clean GPIO | **PROPOSED** |
| `GPIO 15` | `L298N IN4` | Right Reverse | Internal pull-up at boot; `IN3`/`IN4` both HIGH = brake (motors stationary) | **PROPOSED** |
| `GPIO 2` | `Buzzer` (via NPN) | Alert Buzzer | Idle LOW; unplug or ensure LOW during serial flashing | **PROPOSED** |
| `GPIO 4` | `HC-SR04 TRIG` | Ultrasonic Trigger | 10 µs pulse avoids leaving 1W Flash LED ON | **PROPOSED** |
| `GPIO 3` (`U0RXD`) | `HC-SR04 ECHO` | Ultrasonic Echo | **5V→3.3V divider (`1kΩ + 2kΩ`) + unplug before flashing on MB board** | **PROPOSED** |
| `GPIO 1` (`U0TXD`) | `PIR OUT` | Motion Input | **1kΩ series resistor + unplug before flashing; no `Serial` debug logs** | **PROPOSED** |
