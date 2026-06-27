#pragma once

#include "AudioFifo.hpp"
#include "wcet.hpp"

#include <atomic>
#include <vector>

class AudioRecorderSimulator {
private:
    AudioFifo& fifo;
    double phase = 0.0;
    std::atomic<uint64_t> blockCount{0};
    WCETStats genStats;
    WCETStats pushStats;
    WCETStats* e2eStats{nullptr};

public:
    explicit AudioRecorderSimulator(AudioFifo& fifo, WCETStats* e2eStats = nullptr);
    void start();
};

class AudioPlayerSimulator {
private:
    AudioFifo& fifo;
    std::atomic<uint64_t> blockCount{0};
    WCETStats popStats;
    WCETStats consumeStats;
    WCETStats* e2eStats{nullptr};

public:
    explicit AudioPlayerSimulator(AudioFifo& fifo, WCETStats* e2eStats = nullptr);
    void start();
};
