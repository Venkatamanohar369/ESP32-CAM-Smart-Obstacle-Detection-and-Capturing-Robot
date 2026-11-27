#ifndef MOTOR_CONTROLLER_H
#define MOTOR_CONTROLLER_H

#include <Arduino.h>
#include "nano_pins.h"
#include "robot_logic.h"

class MotorController {
public:
    explicit MotorController(uint32_t cmdTimeoutMs = 1200)
        : state_(cmdTimeoutMs) {}

    void begin() {
        pinMode(PIN_L298N_ENA, OUTPUT);
        pinMode(PIN_L298N_ENB, OUTPUT);
        pinMode(PIN_L298N_IN1, OUTPUT);
        pinMode(PIN_L298N_IN2, OUTPUT);
        pinMode(PIN_L298N_IN3, OUTPUT);
        pinMode(PIN_L298N_IN4, OUTPUT);
        applyOutputs(robot_proto::MotorDir::STOP, 0);
    }

    void setCommand(robot_proto::MotorDir dir, uint8_t speed, uint32_t nowMs) {
        state_.setCommand(dir, speed, nowMs);
    }

    void refreshWatchdog(uint32_t nowMs) {
        state_.refreshWatchdog(nowMs);
    }

    void emergencyStop() {
        state_.emergencyStop();
        applyOutputs(robot_proto::MotorDir::STOP, 0);
    }

    void update(uint32_t nowMs, bool obstacleActive) {
        state_.update(nowMs, obstacleActive);
        applyOutputs(state_.activeDir(), state_.currentPwm());
    }

    robot_proto::MotorDir activeDir() const { return state_.activeDir(); }
    uint8_t currentPwm() const { return state_.currentPwm(); }

private:
    robot_logic::MotorSafetyState state_;

    void applyOutputs(robot_proto::MotorDir dir, uint8_t pwm) {
        switch (dir) {
            case robot_proto::MotorDir::FORWARD:
                digitalWrite(PIN_L298N_IN1, HIGH);
                digitalWrite(PIN_L298N_IN2, LOW);
                digitalWrite(PIN_L298N_IN3, HIGH);
                digitalWrite(PIN_L298N_IN4, LOW);
                break;
            case robot_proto::MotorDir::BACKWARD:
                digitalWrite(PIN_L298N_IN1, LOW);
                digitalWrite(PIN_L298N_IN2, HIGH);
                digitalWrite(PIN_L298N_IN3, LOW);
                digitalWrite(PIN_L298N_IN4, HIGH);
                break;
            case robot_proto::MotorDir::LEFT:
                digitalWrite(PIN_L298N_IN1, LOW);
                digitalWrite(PIN_L298N_IN2, HIGH);
                digitalWrite(PIN_L298N_IN3, HIGH);
                digitalWrite(PIN_L298N_IN4, LOW);
                break;
            case robot_proto::MotorDir::RIGHT:
                digitalWrite(PIN_L298N_IN1, HIGH);
                digitalWrite(PIN_L298N_IN2, LOW);
                digitalWrite(PIN_L298N_IN3, LOW);
                digitalWrite(PIN_L298N_IN4, HIGH);
                break;
            case robot_proto::MotorDir::STOP:
            default:
                digitalWrite(PIN_L298N_IN1, LOW);
                digitalWrite(PIN_L298N_IN2, LOW);
                digitalWrite(PIN_L298N_IN3, LOW);
                digitalWrite(PIN_L298N_IN4, LOW);
                pwm = 0;
                break;
        }
        analogWrite(PIN_L298N_ENA, pwm);
        analogWrite(PIN_L298N_ENB, pwm);
    }
};

#endif // MOTOR_CONTROLLER_H
