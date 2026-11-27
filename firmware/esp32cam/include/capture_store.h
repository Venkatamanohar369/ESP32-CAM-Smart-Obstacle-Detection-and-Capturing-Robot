#ifndef CAPTURE_STORE_H
#define CAPTURE_STORE_H

#include <Arduino.h>

struct CapturedImage {
    uint32_t id = 0;
    uint32_t timestampMs = 0;
    int32_t  distanceMm = -1;
    char     reason[12] = {0};
    uint8_t* data = nullptr;
    size_t   len = 0;
};

class CaptureStore {
public:
    static const size_t MAX_CAPTURES = 6;

    void begin() {
        mutex_ = xSemaphoreCreateMutex();
    }

    uint32_t saveCapture(const uint8_t* jpgData, size_t jpgLen, int32_t distMm, const char* reason, uint32_t nowMs) {
        if (!jpgData || jpgLen == 0 || !mutex_) return 0;
        if (xSemaphoreTake(mutex_, pdMS_TO_TICKS(200)) != pdTRUE) return 0;

        uint8_t* copyBuf = static_cast<uint8_t*>(
            psramFound() ? ps_malloc(jpgLen) : malloc(jpgLen)
        );
        if (!copyBuf) {
            xSemaphoreGive(mutex_);
            return 0;
        }
        memcpy(copyBuf, jpgData, jpgLen);

        size_t slot = nextSlot_;
        if (slots_[slot].data) {
            free(slots_[slot].data);
            slots_[slot].data = nullptr;
        }

        slots_[slot].id = ++nextId_;
        slots_[slot].timestampMs = nowMs;
        slots_[slot].distanceMm = distMm;
        strncpy(slots_[slot].reason, reason ? reason : "AUTO", sizeof(slots_[slot].reason) - 1);
        slots_[slot].reason[sizeof(slots_[slot].reason) - 1] = '\0';
        slots_[slot].data = copyBuf;
        slots_[slot].len = jpgLen;

        nextSlot_ = (nextSlot_ + 1) % MAX_CAPTURES;
        if (count_ < MAX_CAPTURES) ++count_;

        uint32_t savedId = slots_[slot].id;
        xSemaphoreGive(mutex_);
        return savedId;
    }

    // Copies metadata list (newest first) without holding lock during HTTP write.
    size_t listMetadata(CapturedImage* outMeta, size_t maxOut) {
        if (!outMeta || maxOut == 0 || !mutex_) return 0;
        if (xSemaphoreTake(mutex_, pdMS_TO_TICKS(100)) != pdTRUE) return 0;

        size_t n = 0;
        for (size_t i = 0; i < count_ && n < maxOut; ++i) {
            size_t idx = (nextSlot_ + MAX_CAPTURES - 1 - i) % MAX_CAPTURES;
            if (slots_[idx].id > 0 && slots_[idx].data) {
                outMeta[n] = slots_[idx];
                outMeta[n].data = nullptr; // Do not expose raw pointer outside lock
                ++n;
            }
        }
        xSemaphoreGive(mutex_);
        return n;
    }

    // Allocates a temporary copy of image bytes for safe HTTP/Telegram transfer.
    bool copyImageById(uint32_t id, uint8_t** outBuf, size_t* outLen, int32_t* outDistMm = nullptr) {
        if (!outBuf || !outLen || !mutex_) return false;
        *outBuf = nullptr;
        *outLen = 0;
        if (xSemaphoreTake(mutex_, pdMS_TO_TICKS(200)) != pdTRUE) return false;

        for (size_t i = 0; i < MAX_CAPTURES; ++i) {
            if (slots_[i].id == id && slots_[i].data && slots_[i].len > 0) {
                uint8_t* tmp = static_cast<uint8_t*>(
                    psramFound() ? ps_malloc(slots_[i].len) : malloc(slots_[i].len)
                );
                if (tmp) {
                    memcpy(tmp, slots_[i].data, slots_[i].len);
                    *outBuf = tmp;
                    *outLen = slots_[i].len;
                    if (outDistMm) *outDistMm = slots_[i].distanceMm;
                }
                xSemaphoreGive(mutex_);
                return (tmp != nullptr);
            }
        }
        xSemaphoreGive(mutex_);
        return false;
    }

    uint32_t latestId() const { return nextId_; }

private:
    SemaphoreHandle_t mutex_ = nullptr;
    CapturedImage slots_[MAX_CAPTURES];
    size_t nextSlot_ = 0;
    size_t count_ = 0;
    uint32_t nextId_ = 0;
};

#endif // CAPTURE_STORE_H
