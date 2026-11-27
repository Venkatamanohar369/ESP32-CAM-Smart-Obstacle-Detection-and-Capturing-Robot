# Wiring Diagram — Option A (`PROPOSED`)

> **SAFETY:** All connections below are **`PROPOSED`** and **NOT YET PHYSICALLY VERIFIED**.  
> **NEVER** connect `12V` to ESP32-CAM, Arduino Nano, `74AHCT125`, PIR, HC-SR04, or the buzzer.  
> **NEVER** power Nano from the `5.0V` Buck rail and USB cable simultaneously.

---

## 1. Signal & Level-Shifting Diagram

```mermaid
flowchart LR
    subgraph ESP["AI-Thinker ESP32-CAM (3.3V Logic)"]
        E14["GPIO 14 (UART2_TX)"]
        E13["GPIO 13 (UART2_RX)"]
    end

    subgraph LVL["Logic-Level Conditioning"]
        BUF["74AHCT125 Buffer<br/>VCC=5.0V, /OE1=GND<br/>(3.3V -> 5.0V)"]
        DIV["Voltage Divider<br/>R1=1kOhm (D9->GPIO13)<br/>R2=2kOhm (GPIO13->GND)<br/>(5.0V -> 3.33V)"]
    end

    subgraph NANO["Arduino Nano (ATmega328P, 5V Logic)"]
        N8["D8 (AltSoftSerial RX)"]
        N9["D9 (AltSoftSerial TX)"]
        N56["D5 (ENA), D6 (ENB)<br/>+ 10k Pull-Downs"]
        N4723["D4/D7 (IN1/IN2)<br/>D2/D3 (IN3/IN4)"]
        N1011["D10 (TRIG), D11 (ECHO)"]
        N12["D12 (PIR OUT)"]
        NA0["A0 (Buzzer Ctrl)"]
    end

    E14 -->|"3.3V"| BUF -->|"5.0V"| N8
    N9 -->|"5.0V"| DIV -->|"3.33V"| E13

    N56 --> L298N["L298N Motor Driver<br/>(5V-EN Jumper REMOVED)"]
    N4723 --> L298N
    L298N -->|"OUT1/2 + 100nF"| ML["Left BO Motor"]
    L298N -->|"OUT3/4 + 100nF"| MR["Right BO Motor"]

    N1011 <--> US["HC-SR04 (5V)"]
    N12 <-- PIR["PIR Sensor"]
    NA0 -->|"1kOhm + NPN + 1N4148"| BUZ["5V Buzzer"]
```

---

## 2. Discrete Circuit Details (`PROPOSED`)

### A. `74AHCT125` Buffer (`ESP32 GPIO 14 → Nano D8`)
```text
  5.0V Buck ────┬──── Pin 14 (VCC)
                │
  ESP32 GPIO14 ─┼──── Pin 2  (1A Input)
  Nano D8 ──────┼──── Pin 3  (1Y Output, 5.0V)
  Star GND ─────┼──── Pin 1  (/1OE Enable, active LOW)
                ├──── Pin 7  (GND)
                └──── Unused inputs (Pin 5, 9, 12) + unused /OE tied to GND
```

### B. Resistor Voltage Divider (`Nano D9 → ESP32 GPIO 13`)
```text
  Nano D9 (5.0V TX) ────[ 1 kΩ ]────┬──── ESP32 GPIO 13 (3.33V RX)
                                    │
                                 [ 2 kΩ ] (or 1.8 kΩ -> 3.21V)
                                    │
                                 Star GND
```

### C. 5V Buzzer Low-Side NPN Driver (`Nano A0`)
```text
  5.0V Buck ────┬──── Buzzer (+)
                │
             [1N4148] (Cathode to 5.0V, Anode to Collector)
                │
                ├──── Buzzer (-)
                │
             Collector
  Nano A0 ──[ 1 kΩ ]── Base (NPN: 2N2222 / BC547 / S8050)
             Emitter
                │
             Star GND
```
