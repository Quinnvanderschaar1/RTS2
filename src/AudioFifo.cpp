#include "AudioFifo.hpp"
#include "TimingLogger.hpp"

#include <chrono>

AudioFifo::AudioFifo() {
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
    queue.push(block);

    // occupancy tracking
    size_t cur = queue.size();
    if (cur > maxSize) maxSize = cur;
    ++occupancyCount;
    cumulativeSize += cur;
    double avg = occupancyCount ? (double)cumulativeSize / occupancyCount : 0.0;

    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);

    FifoSample s{nowNs, std::string("push"), cur, maxSize, avg};
    gTimingLogger.addFifoSample(s);
}

AudioBlock AudioFifo::pop() {
    pthread_mutex_lock(&mutex);

    while (queue.empty()) {
        pthread_cond_wait(&cond, &mutex);
    }

    AudioBlock block = queue.front();
    queue.pop();

    // occupancy tracking after pop
    size_t cur = queue.size();
    if (cur > maxSize) maxSize = cur;
    ++occupancyCount;
    cumulativeSize += cur;
    double avg = occupancyCount ? (double)cumulativeSize / occupancyCount : 0.0;

    pthread_mutex_unlock(&mutex);

    uint64_t nowNs = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();

    FifoSample fs{nowNs, std::string("pop"), cur, maxSize, avg};
    gTimingLogger.addFifoSample(fs);

    if (block.pushNs != 0) {
        QueueLatencySample q{block.pushNs, nowNs, nowNs - block.pushNs};
        gTimingLogger.addQueueLatency(q);
    }

    return block;
}