#include "BeatSync.h"

void BeatSync::updatePeriod(uint32_t now) {
    if (lastBeatMs_ == 0) {
        lastBeatMs_ = now;
        return;
    }

    const uint32_t interval = now - lastBeatMs_;
    lastBeatMs_ = now;

    if (interval < MIN_BEAT_MS || interval > MAX_BEAT_MS) {
        return;
    }

    beatIntervals_[beatIntervalIndex_] = (uint16_t)interval;
    beatIntervalIndex_ = (beatIntervalIndex_ + 1) % BEAT_INTERVAL_HISTORY;
    if (beatIntervalCount_ < BEAT_INTERVAL_HISTORY) {
        beatIntervalCount_++;
    }

    uint32_t sum = 0;
    for (uint8_t i = 0; i < beatIntervalCount_; i++) {
        sum += beatIntervals_[i];
    }
    filteredBeatMs_ = (float)sum / beatIntervalCount_;
}

uint16_t BeatSync::chaseDurationMs() const {
    return (uint16_t)constrain((int32_t)(filteredBeatMs_ + 0.5f), MIN_BEAT_MS, MAX_BEAT_MS);
}
