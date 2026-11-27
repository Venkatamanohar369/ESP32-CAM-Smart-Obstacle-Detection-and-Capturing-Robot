#ifndef PIR_SENSOR_H
#define PIR_SENSOR_H

#include <Arduino.h>
#include "nano_pins.h"

class PirSensor {
public:
    void begin() {
        pinMode(PIN_PIR_OUT, INPUT);
    }

    bool isMotionDetected() const {
        return digitalRead(PIN_PIR_OUT) == HIGH;
    }
};

#endif // PIR_SENSOR_H
