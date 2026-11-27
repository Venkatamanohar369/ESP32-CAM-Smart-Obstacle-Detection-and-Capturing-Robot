#ifndef ULTRASONIC_SENSOR_H
#define ULTRASONIC_SENSOR_H

#include <Arduino.h>
#include "nano_pins.h"
#include "robot_logic.h"

class UltrasonicSensor {
public:
    static const uint32_t ECHO_TIMEOUT_US = 25000UL; // ~4.3 m max, bounds blocking

    void begin() {
        pinMode(PIN_US_TRIG, OUTPUT);
        pinMode(PIN_US_ECHO, INPUT);
        digitalWrite(PIN_US_TRIG, LOW);
    }

    // Performs a bounded HC-SR04 measurement and returns distance in mm (-1 on timeout).
    int32_t readDistanceMm() {
        digitalWrite(PIN_US_TRIG, LOW);
        delayMicroseconds(2);
        digitalWrite(PIN_US_TRIG, HIGH);
        delayMicroseconds(10);
        digitalWrite(PIN_US_TRIG, LOW);

        unsigned long duration = pulseIn(PIN_US_ECHO, HIGH, ECHO_TIMEOUT_US);
        return robot_logic::echoUsToMm(static_cast<uint32_t>(duration));
    }
};

#endif // ULTRASONIC_SENSOR_H
