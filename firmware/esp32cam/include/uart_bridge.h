#ifndef UART_BRIDGE_H
#define UART_BRIDGE_H

#include <Arduino.h>
#include "app_config.h"
#include "uart_protocol.h"

class UartBridge {
public:
    void begin() {
        Serial2.begin(UART_BAUD_RATE, SERIAL_8N1, PIN_UART2_RX, PIN_UART2_TX);
        telemetry_.distanceMm = -1;
        telemetry_.pirTriggered = false;
        telemetry_.obstacleActive = false;
        telemetry_.motorDir = robot_proto::MotorDir::STOP;
        telemetry_.currentPwm = 0;
    }

    void poll(uint32_t nowMs) {
        while (Serial2.available() > 0) {
            char c = static_cast<char>(Serial2.read());
            if (c == '\r') continue;
            if (c == '\n') {
                rxLine_[rxIdx_] = '\0';
                if (rxIdx_ > 0) {
                    char payload[robot_proto::MAX_FRAME_LEN];
                    if (robot_proto::parseFrame(rxLine_, payload, sizeof(payload))) {
                        robot_proto::TelemetryPacket pkt;
                        if (robot_proto::parseTelemetryPayload(payload, pkt)) {
                            telemetry_ = pkt;
                            lastRxMs_ = nowMs;
                            hasTelemetry_ = true;
                        }
                    }
                }
                rxIdx_ = 0;
            } else if (rxIdx_ < sizeof(rxLine_) - 1) {
                rxLine_[rxIdx_++] = c;
            } else {
                rxIdx_ = 0;
            }
        }
    }

    void sendMove(robot_proto::MotorDir dir, uint8_t speed) {
        char payload[32];
        snprintf(payload, sizeof(payload), "CMD,M,%c,%u", robot_proto::dirToChar(dir), static_cast<unsigned>(speed));
        sendPayload(payload);
    }

    void sendBuzzer(uint8_t mode) {
        char payload[24];
        snprintf(payload, sizeof(payload), "CMD,Z,%u", static_cast<unsigned>(mode));
        sendPayload(payload);
    }

    void sendConfig(int obsCm, bool passiveBuzzer) {
        char payload[32];
        snprintf(payload, sizeof(payload), "CMD,CFG,%d,%d", obsCm, passiveBuzzer ? 1 : 0);
        sendPayload(payload);
    }

    void sendPing() {
        sendPayload("CMD,PING");
    }

    bool isNanoOnline(uint32_t nowMs) const {
        return hasTelemetry_ && ((nowMs - lastRxMs_) < 1500UL);
    }

    const robot_proto::TelemetryPacket& telemetry() const { return telemetry_; }

private:
    char rxLine_[robot_proto::MAX_FRAME_LEN] = {0};
    uint8_t rxIdx_ = 0;
    robot_proto::TelemetryPacket telemetry_;
    uint32_t lastRxMs_ = 0;
    bool hasTelemetry_ = false;

    void sendPayload(const char* payload) {
        char frame[robot_proto::MAX_FRAME_LEN];
        if (robot_proto::formatFrame(frame, sizeof(frame), payload) > 0) {
            Serial2.print(frame);
        }
    }
};

#endif // UART_BRIDGE_H
