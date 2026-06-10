#include "TimingLogger.hpp"

#include <fstream>

void TimingLogger::add(const std::string& stage, uint64_t block, uint64_t latencyNs) {
    std::lock_guard<std::mutex> lock(mutex);
    samples.push_back({stage, block, latencyNs});
}

void TimingLogger::addFifoSample(const FifoSample& s) {
    std::lock_guard<std::mutex> lock(mutex);
    fifoSamples.push_back(s);
}

void TimingLogger::addQueueLatency(const QueueLatencySample& s) {
    std::lock_guard<std::mutex> lock(mutex);
    queueSamples.push_back(s);
}

void TimingLogger::addNetworkJitter(const NetworkJitterSample& s) {
    std::lock_guard<std::mutex> lock(mutex);
    networkSamples.push_back(s);
}

void TimingLogger::saveCSV(const std::string& filename) {
    std::lock_guard<std::mutex> lock(mutex);
    std::ofstream file(filename);

    file << "stage,block,latency_ns,latency_ms\n";

    for (const auto& s : samples) {
        file << s.stage << ","
             << s.block << ","
             << s.latencyNs << ","
             << (s.latencyNs / 1e6)
             << "\n";
    }

    file.close();
}

void TimingLogger::saveFifoCSV(const std::string& filename) {
    std::lock_guard<std::mutex> lock(mutex);
    std::ofstream file(filename);
    file << "timestamp_ns,event,current_size,max_size,avg_size\n";
    for (const auto& s : fifoSamples) {
        file << s.tsNs << "," << s.event << "," << s.currentSize << "," << s.maxSize << "," << s.avgSize << "\n";
    }
}

void TimingLogger::saveQueueCSV(const std::string& filename) {
    std::lock_guard<std::mutex> lock(mutex);
    std::ofstream file(filename);
    file << "push_ns,pop_ns,latency_ns\n";
    for (const auto& s : queueSamples) {
        file << s.pushNs << "," << s.popNs << "," << s.latencyNs << "\n";
    }
}

void TimingLogger::saveNetworkCSV(const std::string& filename) {
    std::lock_guard<std::mutex> lock(mutex);
    std::ofstream file(filename);
    file << "send_ns,recv_ns,diff_ns\n";
    for (const auto& s : networkSamples) {
        file << s.sendNs << "," << s.recvNs << "," << s.diffNs << "\n";
    }
}

TimingLogger gTimingLogger;
