# Known Unknowns & Unverified Hardware Parameters

Before final wiring and power-up, inspect and verify the following physical hardware parameters on the bench:

---

## 1. Critical Hardware Unknowns

| # | Unknown Item | Why It Matters | How to Verify |
| :---: | :--- | :--- | :--- |
| **1** | **Target GPIO Wiring** | The ESP32-CAM has limited conflict-free GPIOs once camera, PSRAM, and UART0 are reserved. | Wire according to the approved Architecture Option A pin mapping table. |
| **2** | **2-Motor vs. 4-Wheel Chassis / Caster Requirement** | `PROJECT_BRIEF.md` and user confirmation specify `2 driven single-shaft BO motors`, while [`esp32-cam-robot-project.jpg`](images/esp32-cam-robot-project.jpg) visually shows 4 wheels. A 2-motor chassis tips forward/backward without passive support. | Confirm whether the chassis has 2 passive front wheels or requires adding 1–2 swivel/ball caster wheels for balance and sensor ground clearance. |
| **3** | **Arduino Nano Hardware Variant & Bootloader** | Option A targets **Arduino Nano (`ATmega328P`, 5V/16 MHz)**. Clones vary between old bootloader (`57600` bps) and new bootloader (`115200` bps), and CH340 vs FT232 USB chip. | Confirm Nano is 5V/16 MHz (not 3.3V) and verify upload baud rate (`nanoatmega328` vs `nanoatmega328new`) when flashing. |
| **4** | **PIR Sensor Variant & Output Voltage** | Standard `HC-SR501` runs on 5V and outputs **3.3V** logic (safe for ESP32), whereas some clone PIR boards output **5V** logic. Trigger jumper (`H` repeatable vs `L` single) and pot delays are also unknown. | Measure PIR `OUT` voltage with a multimeter when triggered at 5V `VCC`. |
| **5** | **HC-SR04 Variant** | Classic `HC-SR04` requires **5V `VCC`** and outputs **5V `ECHO`** (needs divider for ESP32). Newer `HC-SR04P` / `RCWL-1601` work at 3.3V. | Check PCB silkscreen (`HC-SR04` vs `HC-SR04P`) and test `ECHO` logic level. |
| **6** | **Buzzer Type (Active vs. Passive)** | An **active 5V buzzer** sounds with DC HIGH; a **passive buzzer** requires a square-wave PWM tone (`tone()`/`ledcWriteTone()`) and stays silent on steady HIGH. | Test briefly across 5V & GND: steady beep = active; single click = passive. |
| **7** | **Buck Converter Model & Current Voltage Setting** | Unknown regulator IC (`LM2596`, `MP1584`, `XL4015`, etc.), current rating, and current trimmer position. If pre-set >5.5V (or 12V pass-through), it will destroy the ESP32-CAM. | Power buck converter alone from 12V and adjust/measure output to **5.0V** with a multimeter first. |
| **8** | **Available Capacitor & Resistor Values** | Listed generally as `Capacitors and resistors`. Exact capacitance (`100µF`, `470µF`, `1000µF`), voltage ratings, and resistor values (`1kΩ`, `2kΩ`, `4.7kΩ`, etc.) are unknown. | Inventory component markings before wiring voltage dividers and decoupling caps. |
| **9** | **NPN Transistor / Diode Availability** | If driving the 5V buzzer from a 3.3V ESP32 GPIO (Options B/C) or high-current buzzer from Arduino, a small NPN transistor (`2N2222`/`BC547`/`S8050`) is needed. | Check parts kit for any general-purpose NPN transistor (or confirm buzzer current <15 mA for Arduino pin). |
| **10** | **OV3660 Module & PSRAM Verification** | Exact FPC ribbon orientation, lens FOV, and actual PSRAM size (4 MB vs 8 MB) cannot be verified until booted. | Run Phase 3/physical boot check and inspect serial log for `OV3660` detection and `psramFound()`. |
| **11** | **BO Motor Exact Voltage & Stall Current** | Standard yellow BO motors are often 3–6V or 6–12V depending on windings. Stall current determines whether L298N + 12V 3A adapter will trip under obstacle stall. | Measure winding resistance or current draw at 9V no-load and brief stall. |
| **12** | **12V Adapter Tethered vs. Battery Operation** | `esp32-cam-robot-project.jpg` shows an AC-to-DC wall adapter sitting on top of the robot. Long AC/DC tether restricts range unless swapped for a 3S LiPo/12V pack later. | Confirm whether robot will run tethered on the 12V 3A adapter during initial testing. |

---

## 2. Firmware & Software Unknowns (to Be Confirmed in Testing)

- **Wi-Fi Network Environment:** 2.4 GHz SSID availability (ESP32 does **not** support 5 GHz Wi-Fi) and whether router/firewall allows outbound HTTPS (`api.telegram.org:443`).
- **Telegram Bot Credentials:** User will supply Bot Token and Chat ID locally in an untracked `config.h` generated from `config.example.h`.
- **OV3660 Image Orientation:** Vertical flip (`vflip`) and horizontal mirror (`hmirror`) settings depend on physical camera mounting orientation on the chassis.
