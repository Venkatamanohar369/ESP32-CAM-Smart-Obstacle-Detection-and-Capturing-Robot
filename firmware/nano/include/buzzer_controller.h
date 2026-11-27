#ifndef BUZZER_CONTROLLER_H
#define BUZZER_CONTROLLER_H

#include <Arduino.h>
#include "nano_pins.h"

class BuzzerController {
public:
    enum class Mode : uint8_t {
        OFF = 0,
        BEEP_ONCE = 1,
        ALARM = 2
    };

    void begin(bool passiveMode = false) {
        passiveMode_ = passiveMode;
        pinMode(PIN_BUZZER, OUTPUT);
        silence();
    }

    void setPassiveMode(bool passive) {
        if (passiveMode_ != passive) {
            silence();
            passiveMode_ = passive;
        }
    }

    void triggerBeep(uint32_t durationMs = 200) {
        mode_ = Mode::BEEP_ONCE;
        beepEndMs_ = millis() + durationMs;
        driveOn();
    }

    void setAlarm(bool on) {
        if (on) {
            mode_ = Mode::ALARM;
        } else if (mode_ == Mode::ALARM) {
            mode_ = Mode::OFF;
            silence();
        }
    }

    void update(uint32_t nowMs) {
        if (mode_ == Mode::BEEP_ONCE) {
            if (static_cast<int32_t>(nowMs - beepEndMs_) >= 0) {
                mode_ = Mode::OFF;
                silence();
            }
        } else if (mode_ == Mode::ALARM) {
            // Non-blocking 2 Hz pulsed alert
            bool pulseOn = ((nowMs / 250UL) % 2UL) == 0;
            if (pulseOn) {
                driveOn();
            } else {
                silence();
            }
        }
    }

private:
    bool passiveMode_ = false;
    Mode mode_ = Mode::OFF;
    uint32_t beepEndMs_ = 0;

    void driveOn() {
        if (passiveMode_) {
            tone(PIN_BUZZER, 2000);
        } else {
            digitalWrite(PIN_BUZZER, HIGH);
        }
    }

    void silence() {
        if (passiveMode_) {
            noTone(PIN_BUZZER);
        }
        digitalWrite(PIN_BUZZER, LOW);
    }
};

#endif // BUZZER_CONTROLLER_H
