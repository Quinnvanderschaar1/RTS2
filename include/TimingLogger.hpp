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
    std::string fifoName;
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

struct DropSample {
    uint64_t tsNs;
    std::string fifoName;
    std::string process;
    std::string reason;
};

class TimingLogger {
public:
    void add(const std::string& stage, uint64_t block, uint64_t latencyNs);

    void addFifoSample(const FifoSample& s);
    void addQueueLatency(const QueueLatencySample& s);
    void addNetworkJitter(const NetworkJitterSample& s);
    void addDropSample(const DropSample& s);

    void saveCSV(const std::string& filename);
    void saveFifoCSV(const std::string& filename);
    void saveQueueCSV(const std::string& filename);
    void saveNetworkCSV(const std::string& filename);
    void saveDropCSV(const std::string& filename);

private:
    std::vector<TimingSample> samples;
    std::vector<FifoSample> fifoSamples;
    std::vector<QueueLatencySample> queueSamples;
    std::vector<NetworkJitterSample> networkSamples;
    std::vector<DropSample> dropSamples;
    std::mutex mutex;
};

extern TimingLogger gTimingLogger;
