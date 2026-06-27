#pragma once

#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

struct TimingSample {
    std::string stage;
    uint64_t block;
    uint64_t latencyNs;
};

struct FifoSample {
    uint64_t tsNs;
    std::string event; // "push" or "pop"
    size_t currentSize;
    size_t maxSize;
    double avgSize;
};

struct QueueLatencySample {
    uint64_t pushNs;
    uint64_t popNs;
    uint64_t latencyNs;
};

struct NetworkJitterSample {
    uint64_t sendNs;
    uint64_t recvNs;
    int64_t diffNs;
};

class TimingLogger {
public:
    void add(const std::string& stage, uint64_t block, uint64_t latencyNs);

    void addFifoSample(const FifoSample& s);
    void addQueueLatency(const QueueLatencySample& s);
    void addNetworkJitter(const NetworkJitterSample& s);

    void saveCSV(const std::string& filename);
    void saveFifoCSV(const std::string& filename);
    void saveQueueCSV(const std::string& filename);
    void saveNetworkCSV(const std::string& filename);

private:
    std::vector<TimingSample> samples;
    std::vector<FifoSample> fifoSamples;
    std::vector<QueueLatencySample> queueSamples;
    std::vector<NetworkJitterSample> networkSamples;
    std::mutex mutex;
};

extern TimingLogger gTimingLogger;
