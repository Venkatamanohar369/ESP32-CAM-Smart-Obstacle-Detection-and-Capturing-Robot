# Bill of Materials (BOM) — ESP32-CAM Autonomous Surveillance Robot

> **Document Status:** PHASE 1 REVISION (`Option A` Architecture Conditionally Approved; All Connections `PROPOSED`)  
> All specifications, quantities, ratings, and ownership/verification statuses are cross-checked against [`docs/GPIO_FEASIBILITY.md`](../docs/GPIO_FEASIBILITY.md) and [`docs/POWER_DESIGN.md`](../docs/POWER_DESIGN.md).

---

## 1. Components Already Owned

| # | Component | Qty | Known Spec | Unverified / Unknown Specs (Verify Before Use) | Purpose |
| :---: | :--- | :---: | :--- | :--- | :--- |
| **1** | **AI-Thinker ESP32-CAM** | 1 | ESP32-S, 24-pin FPC, `3.3V` logic / `5.0V` power in | Exact PSRAM size (`4 MB` typical) | Camera streaming, web dashboard, Wi-Fi, Telegram |
| **2** | **OV3660 Camera Module** | 1 | 3MP 24-pin 0.5 mm FPC | Lens FOV & mounting orientation (`vflip`/`hmirror`) | Video streaming & motion-triggered capture |
| **3** | **ESP32-CAM-MB Programmer** | 1 | CH340C USB-UART + `IO0`/`RST` buttons | — | Firmware flashing & `UART0` debug console |
| **4** | **Arduino Nano** | 1 | ATmega328P, `5V` / `16 MHz` | Clone USB chip & bootloader (`57600` vs `115200`) | Real-time motor PWM, ultrasonic, PIR, buzzer |
| **5** | **PIR Motion Sensor** | 1 | Digital motion detector | Exact model (`HC-SR501` vs clone) & `OUT` voltage (`3.3V` vs `5.0V`) | Human motion detection (`Nano D12`) |
| **6** | **HC-SR04 Ultrasonic Sensor** | 1 | `5V` ultrasonic distance sensor | Exact variant (`HC-SR04` vs `HC-SR04P`) | Distance measurement & emergency stop (`Nano D10`/`D11`) |
| **7** | **L298N Dual H-Bridge Module** | 1 | Dual H-bridge, `2 A/ch` DC max, onboard `78M05` | Exact heatsink thermal margin | Drives 2× BO motors (`5V-EN` jumper **REMOVED**) |
| **8** | **Single-Shaft BO Motors** | 2 | Geared DC motor, rated `6–12 V` | **Winding resistance & stall current UNKNOWN** | Left & right differential drive |
| **9** | **Small 5V Buzzer** | 1 | `5V` nominal | **Active DC vs. Passive piezo/magnetic UNKNOWN** | Obstacle & motion audio alert (`Nano A0` via NPN) |
| **10** | **12V, 3A DC Adapter** | 1 | `12.0 V`, `3.0 A` (`36 W`) | Exact polarity & load droop | Main robot power supply |
| **11** | **Buck Converter** | 1 | Step-down DC-DC | **Exact IC/model & continuous current rating (must be ≥ `2.0 A`) UNKNOWN** | Steps `12V → 5.00V ± 0.10V` for logic rail |
| **12** | **Capacitors & Resistors** | Kit | Assorted values | Exact capacitance, voltage ratings, and resistor values | Check against Section 2 before purchasing |
| **13** | **Robot Chassis & Wires** | 1 set | 2 driven motor mounts | Whether 2 passive wheels exist or 1 caster is needed | Mechanical platform & signal wiring |

---

## 2. Components That Must Be Purchased (or Verified Missing from Kit)

| # | Component | Qty | Exact Specification & Rating | Category | Purpose |
| :---: | :--- | :---: | :--- | :--- | :--- |
| **P1** | **Logic Level Buffer IC** | 1 | **`74AHCT125`** (or `74HCT125` / `74AHCT1G125`), DIP-14 or breakout, `VCC = 5.0V`, `VIH(min) = 2.0V` *(or 1× `BSS138` 3.3V↔5V module)* | **Purchase Required** (Level Shifting) | Translates `ESP32 GPIO 14 TX (3.3V) → Nano D8 RX (5.0V)` cleanly with `~2.0V` noise margin |
| **P2** | **Voltage Divider Resistors** | 2 | **`1 kΩ`** (1/4 W, 1% or 5%) + **`2 kΩ`** (or `1.8 kΩ`, 1/4 W) | Verify in kit / Purchase | Mandatory `5.0V → 3.33V` (or `3.21V`) divider on `Nano D9 TX → ESP32 GPIO 13 RX` |
| **P3** | **NPN Transistor** | 1 | **`2N2222`**, **`BC547`**, or **`S8050`** (`≥ 30V`, `≥ 200 mA`) | Verify in kit / Purchase | Low-side switch isolating 5V buzzer load from `Nano A0` |
| **P4** | **Flyback Diode** | 1 | **`1N4148`** (fast signal) or **`1N4007`** | Verify in kit / Purchase | Clamps inductive flyback across 5V buzzer |
| **P5** | **Base & Pull-Down Resistors** | 3 | `1 kΩ` (×1, 1/4 W) + `10 kΩ` (×2, 1/4 W) | Verify in kit / Purchase | `1 kΩ` buzzer NPN base resistor; `10 kΩ` pull-downs on `L298N ENA`/`ENB` against Nano boot twitch |
| **P6** | **12V Bus Bulk Capacitor** | 1 | **`470 µF – 1000 µF` Electrolytic, ≥ `25 V`** | Verify in kit / Purchase | Buffers motor inrush & inductive spikes at L298N `+12V` |
| **P7** | **5V Rail / ESP32 Bulk Cap** | 1–2 | **`470 µF` Low-ESR Electrolytic, ≥ `10 V` (`16 V` pref.)** | Verify in kit / Purchase | Prevents ESP32-CAM brownouts during Wi-Fi TX + OV3660 capture |
| **P8** | **Ceramic Decoupling Caps** | 5 | **`100 nF` (`0.1 µF`) Ceramic, ≥ `25 V`** | Verify in kit / Purchase | 1× 12V bus, 1× ESP32-CAM `5V`, 1× Nano `5V`, 1× across each BO motor |
| **P9** | **Inline Fuse + Holder** | 1 | **`3.0 A` Fast-Blow Fuse** (`5×20 mm` or blade) + inline holder (**PROVISIONAL**) | **Purchase Required** (Protection) | Provisional protection for `12V 3A` adapter & wiring pending time-current curve and wire-gauge verification |
| **P10** | **Main Power Switch** | 1 | **SPST Toggle / Rocker Switch, ≥ `12 V / 5 A`** | Recommended (Protection) | Clean manual power cut-off on `12V` input |
| **P11** | **Power Hookup Wire** | 1–2 m | **`20–22 AWG` Stranded Copper** (Red/Black) | Recommended (Wiring) | Low-drop star power & star ground distribution |
| **P12** | **Swivel / Ball Caster Wheel** | 1 | Standard chassis ball caster | **Conditional** | Required ONLY if physical chassis has only 2 wheels (no 2 passive front wheels) |
