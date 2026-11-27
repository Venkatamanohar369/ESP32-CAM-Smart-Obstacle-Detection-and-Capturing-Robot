#ifndef ROBOT_LOGIC_H
#define ROBOT_LOGIC_H

#include <stdint.h>
#include "uart_protocol.h"

namespace robot_logic {

using robot_proto::MotorDir;

// Converts HC-SR04 echo duration in microseconds to millimeters.
// Returns -1 on timeout (0 us) or out-of-range (> 4000 mm / < 20 mm).
inline int32_t echoUsToMm(uint32_t echoUs) {
    if (echoUs == 0) return -1;
    // Speed of sound ~0.343 mm/us -> round trip ~5.83 us/mm -> us * 10 / 58
    int32_t mm = static_cast<int32_t>((echoUs * 10UL) / 58UL);
    if (mm < 20 || mm > 4000) return -1;
    return mm;
}

class ObstacleDetector {
public:
    explicit ObstacleDetector(int32_t stopThresholdMm = 200, int32_t hysteresisMm = 30)
        : thresholdMm_(stopThresholdMm),
          hysteresisMm_(hysteresisMm),
          active_(false),
          triggerHits_(0),
          clearHits_(0) {}

    void setThresholdMm(int32_t mm) {
        if (mm >= 50 && mm <= 2000) {
            thresholdMm_ = mm;
        }
    }

    int32_t thresholdMm() const { return thresholdMm_; }
    bool isObstacleActive() const { return active_; }

    // Feeds a distance reading in mm (-1 = timeout/invalid).
    // Returns true if obstacle emergency stop is active.
    bool update(int32_t distMm) {
        if (distMm < 0) {
            // Do not falsely clear an active obstacle on a single missed echo.
            triggerHits_ = 0;
            return active_;
        }

        if (!active_) {
            if (distMm <= thresholdMm_) {
                if (++triggerHits_ >= 2) {
                    active_ = true;
                    clearHits_ = 0;
                }
            } else {
                triggerHits_ = 0;
            }
        } else {
            if (distMm >= (thresholdMm_ + hysteresisMm_)) {
                if (++clearHits_ >= 2) {
                    active_ = false;
                    triggerHits_ = 0;
                }
            } else {
                clearHits_ = 0;
            }
        }
        return active_;
    }

private:
    int32_t thresholdMm_;
    int32_t hysteresisMm_;
    bool active_;
    uint8_t triggerHits_;
    uint8_t clearHits_;
};

class MotorSafetyState {
public:
    static const uint32_t DEFAULT_DEADTIME_MS = 100;
    static const uint8_t  RAMP_STEP_PWM       = 20;
    static const uint32_t RAMP_INTERVAL_MS    = 20;

    explicit MotorSafetyState(uint32_t cmdTimeoutMs = 1200)
        : cmdTimeoutMs_(cmdTimeoutMs),
          activeDir_(MotorDir::STOP),
          targetDir_(MotorDir::STOP),
          currentPwm_(0),
          targetPwm_(0),
          inDeadtime_(false),
          deadtimeStartMs_(0),
          lastCmdMs_(0),
          lastRampMs_(0) {}

    void setCommand(MotorDir dir, uint8_t speed, uint32_t nowMs) {
        lastCmdMs_ = nowMs;
        if (dir == MotorDir::STOP || speed == 0) {
            targetDir_ = MotorDir::STOP;
            targetPwm_ = 0;
            activeDir_ = MotorDir::STOP;
            currentPwm_ = 0;
            inDeadtime_ = false;
            return;
        }

        if (dir != activeDir_ && activeDir_ != MotorDir::STOP) {
            // Enforce coast dead-time before switching motor polarity
            activeDir_ = MotorDir::STOP;
            currentPwm_ = 0;
            inDeadtime_ = true;
            deadtimeStartMs_ = nowMs;
        }
        targetDir_ = dir;
        targetPwm_ = speed;
    }

    void refreshWatchdog(uint32_t nowMs) {
        lastCmdMs_ = nowMs;
    }

    void emergencyStop() {
        activeDir_ = MotorDir::STOP;
        targetDir_ = MotorDir::STOP;
        currentPwm_ = 0;
        targetPwm_ = 0;
        inDeadtime_ = false;
    }

    void update(uint32_t nowMs, bool obstacleActive) {
        // Watchdog timeout: stop motors if coordinator stops refreshing
        if (activeDir_ != MotorDir::STOP || targetDir_ != MotorDir::STOP) {
            if (nowMs - lastCmdMs_ >= cmdTimeoutMs_) {
                emergencyStop();
                return;
            }
        }

        // Emergency stop: block forward motion when obstacle is detected
        if (obstacleActive && (activeDir_ == MotorDir::FORWARD || targetDir_ == MotorDir::FORWARD)) {
            emergencyStop();
            return;
        }

        if (inDeadtime_) {
            if (nowMs - deadtimeStartMs_ >= DEFAULT_DEADTIME_MS) {
                inDeadtime_ = false;
                activeDir_ = targetDir_;
                currentPwm_ = 0;
                lastRampMs_ = nowMs;
            }
            return;
        }

        if (activeDir_ == MotorDir::STOP && targetDir_ != MotorDir::STOP) {
            activeDir_ = targetDir_;
            currentPwm_ = 0;
            lastRampMs_ = nowMs;
        }

        if (activeDir_ == MotorDir::STOP) {
            currentPwm_ = 0;
            return;
        }

        // Soft-start PWM ramp
        if (nowMs - lastRampMs_ >= RAMP_INTERVAL_MS) {
            lastRampMs_ = nowMs;
            if (currentPwm_ < targetPwm_) {
                uint16_t next = static_cast<uint16_t>(currentPwm_) + RAMP_STEP_PWM;
                currentPwm_ = (next > targetPwm_) ? targetPwm_ : static_cast<uint8_t>(next);
            } else if (currentPwm_ > targetPwm_) {
                currentPwm_ = targetPwm_;
            }
        }
    }

    MotorDir activeDir() const { return activeDir_; }
    uint8_t currentPwm() const { return currentPwm_; }
    bool isInDeadtime() const { return inDeadtime_; }

private:
    uint32_t cmdTimeoutMs_;
    MotorDir activeDir_;
    MotorDir targetDir_;
    uint8_t  currentPwm_;
    uint8_t  targetPwm_;
    bool     inDeadtime_;
    uint32_t deadtimeStartMs_;
    uint32_t lastCmdMs_;
    uint32_t lastRampMs_;
};

class PirCooldown {
public:
    explicit PirCooldown(uint32_t cooldownMs = 8000UL)
        : cooldownMs_(cooldownMs), prevState_(false), lastTriggerMs_(0), hasTriggered_(false) {}

    // Returns true only on a valid rising edge outside cooldown.
    bool update(bool pirHigh, uint32_t nowMs) {
        bool fire = false;
        if (pirHigh && !prevState_) {
            if (!hasTriggered_ || (nowMs - lastTriggerMs_ >= cooldownMs_)) {
                fire = true;
                lastTriggerMs_ = nowMs;
                hasTriggered_ = true;
            }
        }
        prevState_ = pirHigh;
        return fire;
    }

private:
    uint32_t cooldownMs_;
    bool prevState_;
    uint32_t lastTriggerMs_;
    bool hasTriggered_;
};

} // namespace robot_logic

#endif // ROBOT_LOGIC_H
