#ifndef UART_PROTOCOL_H
#define UART_PROTOCOL_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

namespace robot_proto {

static const uint8_t MAX_FRAME_LEN = 64;

enum class MotorDir : uint8_t {
    STOP = 0,
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT
};

inline char dirToChar(MotorDir dir) {
    switch (dir) {
        case MotorDir::FORWARD:  return 'F';
        case MotorDir::BACKWARD: return 'B';
        case MotorDir::LEFT:     return 'L';
        case MotorDir::RIGHT:    return 'R';
        case MotorDir::STOP:
        default:                 return 'S';
    }
}

inline MotorDir charToDir(char c) {
    switch (c) {
        case 'F': case 'f': return MotorDir::FORWARD;
        case 'B': case 'b': return MotorDir::BACKWARD;
        case 'L': case 'l': return MotorDir::LEFT;
        case 'R': case 'r': return MotorDir::RIGHT;
        case 'S': case 's':
        default:            return MotorDir::STOP;
    }
}

inline uint8_t computeXorChecksum(const char* payload, size_t len) {
    uint8_t cs = 0;
    for (size_t i = 0; i < len; ++i) {
        cs ^= static_cast<uint8_t>(payload[i]);
    }
    return cs;
}

inline uint8_t hexNibble(char c) {
    if (c >= '0' && c <= '9') return static_cast<uint8_t>(c - '0');
    if (c >= 'A' && c <= 'F') return static_cast<uint8_t>(c - 'A' + 10);
    if (c >= 'a' && c <= 'f') return static_cast<uint8_t>(c - 'a' + 10);
    return 0xFF;
}

// Formats "$<payload>*<XX>\n" into outBuf. Returns length or 0 if buffer too small.
inline size_t formatFrame(char* outBuf, size_t outSize, const char* payload) {
    if (!outBuf || !payload) return 0;
    size_t plen = strlen(payload);
    // '$' + payload + '*' + 2 hex + '\n' + '\0'
    if (plen + 6 > outSize) return 0;
    uint8_t cs = computeXorChecksum(payload, plen);
    int n = snprintf(outBuf, outSize, "$%s*%02X\n", payload, cs);
    return (n > 0 && static_cast<size_t>(n) < outSize) ? static_cast<size_t>(n) : 0;
}

// Validates "$<payload>*<XX>" (with or without trailing \r/\n) and extracts payload.
inline bool parseFrame(const char* line, char* outPayload, size_t outSize) {
    if (!line || !outPayload || outSize < 2) return false;
    size_t len = strlen(line);
    while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
        --len;
    }
    if (len < 5 || line[0] != '$') return false;
    if (line[len - 3] != '*') return false;

    uint8_t hi = hexNibble(line[len - 2]);
    uint8_t lo = hexNibble(line[len - 1]);
    if (hi == 0xFF || lo == 0xFF) return false;
    uint8_t expected = static_cast<uint8_t>((hi << 4) | lo);

    size_t plen = len - 4; // exclude '$', '*', and 2 hex chars
    if (plen + 1 > outSize) return false;

    uint8_t actual = computeXorChecksum(line + 1, plen);
    if (actual != expected) return false;

    memcpy(outPayload, line + 1, plen);
    outPayload[plen] = '\0';
    return true;
}

struct TelemetryPacket {
    int32_t distanceMm; // -1 if invalid/timeout
    bool pirTriggered;
    bool obstacleActive;
    MotorDir motorDir;
    uint8_t currentPwm;
};

inline bool parseTelemetryPayload(const char* payload, TelemetryPacket& out) {
    // Expected: TEL,<dist_mm>,<pir>,<obs>,<dir>,<pwm>
    if (!payload || strncmp(payload, "TEL,", 4) != 0) return false;
    char buf[MAX_FRAME_LEN];
    strncpy(buf, payload + 4, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    char* p1 = buf;
    char* p2 = strchr(p1, ','); if (!p2) return false; *p2++ = '\0';
    char* p3 = strchr(p2, ','); if (!p3) return false; *p3++ = '\0';
    char* p4 = strchr(p3, ','); if (!p4) return false; *p4++ = '\0';
    char* p5 = strchr(p4, ','); if (!p5) return false; *p5++ = '\0';

    out.distanceMm = atol(p1);
    out.pirTriggered = (atoi(p2) != 0);
    out.obstacleActive = (atoi(p3) != 0);
    out.motorDir = charToDir(p4[0]);
    int pwmVal = atoi(p5);
    if (pwmVal < 0) pwmVal = 0;
    if (pwmVal > 255) pwmVal = 255;
    out.currentPwm = static_cast<uint8_t>(pwmVal);
    return true;
}

} // namespace robot_proto

#endif // UART_PROTOCOL_H
