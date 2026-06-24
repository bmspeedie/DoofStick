#ifndef DOOF_BEAT_SYNC_H
#define DOOF_BEAT_SYNC_H

#include <Arduino.h>
#include "config.h"

class BeatSync {
public:
    void updatePeriod(uint32_t now);
    uint16_t chaseDurationMs() const;

private:
    float filteredBeatMs_ = DEFAULT_CHASE_MS;
    uint32_t lastBeatMs_ = 0;
    uint16_t beatIntervals_[BEAT_INTERVAL_HISTORY] = {0, 0};
    uint8_t beatIntervalCount_ = 0;
    uint8_t beatIntervalIndex_ = 0;
};

#endif
