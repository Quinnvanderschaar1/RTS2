#include "AudioFifo.hpp"
#include "TimingLogger.hpp"

#include <chrono>

AudioFifo::AudioFifo(size_t capacity_, const std::string& name)
    : fifoName(name), capacity(capacity_) {
    pthread_mutex_init(&mutex, nullptr);
    pthread_cond_init(&cond, nullptr);
}

AudioFifo::~AudioFifo() {
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);
}

void AudioFifo::push(AudioBlock block) {
    uint64_t nowNs = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();

    block.pushNs = nowNs;

    pthread_mutex_lock(&mutex);
    while (queue.size() >= capacity) {
        pthread_cond_wait(&cond, &mutex);
    }
    queue.push(block);

    // occupancy tracking
    size_t cur = queue.size();
    if (cur > maxSize) maxSize = cur;
    ++occupancyCount;
    cumulativeSize += cur;
    double avg = occupancyCount ? (double)cumulativeSize / occupancyCount : 0.0;

    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);

    FifoSample s{nowNs, fifoName, std::string("push"), cur, maxSize, avg};
    gTimingLogger.addFifoSample(s);
}

AudioBlock AudioFifo::pop() {
    pthread_mutex_lock(&mutex);

    while (queue.empty()) {
        pthread_cond_wait(&cond, &mutex);
    }

    AudioBlock block = queue.front();
    queue.pop();
    pthread_cond_signal(&cond);

    // occupancy tracking after pop
    size_t cur = queue.size();
    if (cur > maxSize) maxSize = cur;
    ++occupancyCount;
    cumulativeSize += cur;
    double avg = occupancyCount ? (double)cumulativeSize / occupancyCount : 0.0;

    pthread_mutex_unlock(&mutex);

    uint64_t nowNs = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();

    FifoSample fs{nowNs, fifoName, std::string("pop"), cur, maxSize, avg};
    gTimingLogger.addFifoSample(fs);

    if (block.pushNs != 0) {
        QueueLatencySample q{block.pushNs, nowNs, nowNs - block.pushNs};
        gTimingLogger.addQueueLatency(q);
    }

    return block;
}

bool AudioFifo::tryPush(AudioBlock block, bool log) {
    uint64_t nowNs = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();

    block.pushNs = nowNs;
    if (pthread_mutex_trylock(&mutex) != 0) {
        return false;
    }

    if (queue.size() >= capacity) {
        pthread_mutex_unlock(&mutex);
        gTimingLogger.addDropSample({nowNs, fifoName, "unknown", "fifo_full"});
        return false;
    }

    queue.push(block);
    size_t cur = queue.size();
    if (cur > maxSize) maxSize = cur;
    ++occupancyCount;
    cumulativeSize += cur;
    double avg = occupancyCount ? (double)cumulativeSize / occupancyCount : 0.0;

    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);

    if (log) {
        FifoSample s{nowNs, fifoName, std::string("push"), cur, maxSize, avg};
        gTimingLogger.addFifoSample(s);
    }
    return true;
}

bool AudioFifo::tryPop(AudioBlock& block, bool log) {
    if (pthread_mutex_trylock(&mutex) != 0) {
        return false;
    }

    if (queue.empty()) {
        pthread_mutex_unlock(&mutex);
        return false;
    }

    block = queue.front();
    queue.pop();
    pthread_cond_signal(&cond);

    size_t cur = queue.size();
    if (cur > maxSize) maxSize = cur;
    ++occupancyCount;
    cumulativeSize += cur;
    double avg = occupancyCount ? (double)cumulativeSize / occupancyCount : 0.0;

    pthread_mutex_unlock(&mutex);

    uint64_t nowNs = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();

    if (log) {
        FifoSample fs{nowNs, fifoName, std::string("pop"), cur, maxSize, avg};
        gTimingLogger.addFifoSample(fs);

        if (block.pushNs != 0) {
            QueueLatencySample q{block.pushNs, nowNs, nowNs - block.pushNs};
            gTimingLogger.addQueueLatency(q);
        }
    }

    return true;
}
