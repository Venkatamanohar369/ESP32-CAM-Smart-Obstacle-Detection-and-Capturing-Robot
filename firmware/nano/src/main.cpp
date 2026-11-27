#include <Arduino.h>
#include <AltSoftSerial.h>

#include "nano_pins.h"
#include "uart_protocol.h"
#include "robot_logic.h"
#include "motor_controller.h"
#include "ultrasonic_sensor.h"
#include "pir_sensor.h"
#include "buzzer_controller.h"

namespace {

AltSoftSerial espSerial; // Fixed AVR Timer1: D8=RX, D9=TX
MotorController motors(1200);
UltrasonicSensor ultrasonic;
PirSensor pir;
BuzzerController buzzer;
robot_logic::ObstacleDetector obstacleDetector(200, 30);

char rxLine[robot_proto::MAX_FRAME_LEN];
uint8_t rxIdx = 0;

int32_t lastDistMm = -1;
bool lastPir = false;
bool prevObstacle = false;

uint32_t lastSensorMs = 0;
uint32_t lastTelMs = 0;
const uint32_t SENSOR_INTERVAL_MS = 60;
const uint32_t TELEMETRY_INTERVAL_MS = 100;

void handleCommandPayload(const char* payload, uint32_t nowMs) {
    if (strncmp(payload, "CMD,M,", 6) == 0) {
        char dirChar = payload[6];
        int speed = 0;
        const char* comma = strchr(payload + 6, ',');
        if (comma) {
            speed = atoi(comma + 1);
            if (speed < 0) speed = 0;
            if (speed > 255) speed = 255;
        }
        robot_proto::MotorDir dir = robot_proto::charToDir(dirChar);
        if (obstacleDetector.isObstacleActive() && dir == robot_proto::MotorDir::FORWARD) {
            motors.emergencyStop();
            buzzer.triggerBeep(150);
        } else {
            motors.setCommand(dir, static_cast<uint8_t>(speed), nowMs);
        }
    } else if (strncmp(payload, "CMD,Z,", 6) == 0) {
        int mode = atoi(payload + 6);
        if (mode == 0) {
            buzzer.setAlarm(false);
        } else if (mode == 1) {
            buzzer.triggerBeep(200);
        } else if (mode == 2) {
            buzzer.setAlarm(true);
        }
    } else if (strncmp(payload, "CMD,CFG,", 8) == 0) {
        // CMD,CFG,<obs_cm>,<passive_buz>
        int obsCm = atoi(payload + 8);
        const char* comma = strchr(payload + 8, ',');
        if (obsCm >= 5 && obsCm <= 200) {
            obstacleDetector.setThresholdMm(static_cast<int32_t>(obsCm) * 10);
        }
        if (comma) {
            buzzer.setPassiveMode(atoi(comma + 1) != 0);
        }
    } else if (strcmp(payload, "CMD,PING") == 0) {
        motors.refreshWatchdog(nowMs);
    }
}

void pollUart(uint32_t nowMs) {
    while (espSerial.available() > 0) {
        char c = static_cast<char>(espSerial.read());
        if (c == '\r') continue;
        if (c == '\n') {
            rxLine[rxIdx] = '\0';
            if (rxIdx > 0) {
                char payload[robot_proto::MAX_FRAME_LEN];
                if (robot_proto::parseFrame(rxLine, payload, sizeof(payload))) {
                    handleCommandPayload(payload, nowMs);
                }
            }
            rxIdx = 0;
        } else if (rxIdx < sizeof(rxLine) - 1) {
            rxLine[rxIdx++] = c;
        } else {
            rxIdx = 0; // Overflow reset
        }
    }
}

void sendTelemetry() {
    char payload[robot_proto::MAX_FRAME_LEN];
    char frame[robot_proto::MAX_FRAME_LEN];
    snprintf(
        payload,
        sizeof(payload),
        "TEL,%ld,%d,%d,%c,%u",
        static_cast<long>(lastDistMm),
        lastPir ? 1 : 0,
        obstacleDetector.isObstacleActive() ? 1 : 0,
        robot_proto::dirToChar(motors.activeDir()),
        static_cast<unsigned>(motors.currentPwm())
    );
    if (robot_proto::formatFrame(frame, sizeof(frame), payload) > 0) {
        espSerial.print(frame);
    }
}

} // namespace

void setup() {
    Serial.begin(115200); // Free on D0/D1 for local debug
    espSerial.begin(38400);
    // Disable internal 5V pull-up enabled by AltSoftSerial on D8; 74AHCT125 drives D8 cleanly.
    digitalWrite(PIN_UART_RX, LOW);

    motors.begin();
    ultrasonic.begin();
    pir.begin();
    buzzer.begin(false);

    Serial.println(F("[NANO] Safe startup complete. Motors stopped."));
}

void loop() {
    uint32_t nowMs = millis();

    pollUart(nowMs);

    if (nowMs - lastSensorMs >= SENSOR_INTERVAL_MS) {
        lastSensorMs = nowMs;
        lastDistMm = ultrasonic.readDistanceMm();
        lastPir = pir.isMotionDetected();

        bool obs = obstacleDetector.update(lastDistMm);
        if (obs && !prevObstacle) {
            motors.emergencyStop();
            buzzer.setAlarm(true);
        } else if (!obs && prevObstacle) {
            buzzer.setAlarm(false);
        }
        prevObstacle = obs;
    }

    motors.update(nowMs, obstacleDetector.isObstacleActive());
    buzzer.update(nowMs);

    if (nowMs - lastTelMs >= TELEMETRY_INTERVAL_MS) {
        lastTelMs = nowMs;
        sendTelemetry();
    }
}
