#ifndef NANO_PINS_H
#define NANO_PINS_H

// ============================================================================
// PROPOSED Arduino Nano (ATmega328P, 5V/16MHz) Pin Assignments (Option A)
// Source: docs/GPIO_FEASIBILITY.md — Mark as PROPOSED until physically verified.
// ============================================================================

// UART link to ESP32-CAM via AltSoftSerial (Timer1: D8=RX, D9=TX)
// - D8 receives from ESP32 GPIO 14 via 74AHCT125/74HCT125 3.3V->5V buffer
// - D9 transmits to ESP32 GPIO 13 via 1k + 2k resistor voltage divider
static const uint8_t PIN_UART_RX = 8;
static const uint8_t PIN_UART_TX = 9;

// L298N Dual H-Bridge Motor Driver (5V-EN jumper REMOVED)
static const uint8_t PIN_L298N_ENA = 5;  // Timer0 PWM (10k pull-down to GND)
static const uint8_t PIN_L298N_ENB = 6;  // Timer0 PWM (10k pull-down to GND)
static const uint8_t PIN_L298N_IN1 = 4;  // Left motor A
static const uint8_t PIN_L298N_IN2 = 7;  // Left motor B
static const uint8_t PIN_L298N_IN3 = 2;  // Right motor A
static const uint8_t PIN_L298N_IN4 = 3;  // Right motor B

// HC-SR04 Ultrasonic Distance Sensor (5V)
static const uint8_t PIN_US_TRIG = 10;   // Digital output (no PWM needed)
static const uint8_t PIN_US_ECHO = 11;   // Digital input

// PIR Motion Sensor
static const uint8_t PIN_PIR_OUT = 12;   // D13 avoided (bootloader LED blink)

// 5V Buzzer (via 1k resistor + NPN transistor + flyback diode)
static const uint8_t PIN_BUZZER  = A0;   // Supports digitalWrite (active) & tone() (passive)

#endif // NANO_PINS_H
