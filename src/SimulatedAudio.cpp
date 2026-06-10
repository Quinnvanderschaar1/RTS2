#include "SimulatedAudio.hpp"
#include <chrono>
#include <cmath>
#include <thread>
#include <iostream>

constexpr int SAMPLE_RATE = 48000;
constexpr int FRAMES_10MS = SAMPLE_RATE / 100;
constexpr int BLOCK_SIZE = FRAMES_10MS;
constexpr double TWO_PI = 6.28318530717958647692;
constexpr double SIM_TONE_FREQUENCY = 440.0;

AudioRecorderSimulator::AudioRecorderSimulator(AudioFifo& f, WCETStats* e2e) : fifo(f), e2eStats(e2e) {}

void AudioRecorderSimulator::start() {
    const auto blockDuration = std::chrono::microseconds(1000000LL * FRAMES_10MS / SAMPLE_RATE);
    auto nextWake = std::chrono::steady_clock::now();
    std::vector<float> buffer(BLOCK_SIZE);

    while (true) {
        auto t0 = std::chrono::steady_clock::now();
        for (int i = 0; i < BLOCK_SIZE; ++i) {
            buffer[i] = 0.5f * static_cast<float>(std::sin(TWO_PI * SIM_TONE_FREQUENCY * phase / SAMPLE_RATE));
            phase += 1.0;
            if (phase >= SAMPLE_RATE) phase -= SAMPLE_RATE;
        }
        auto t1 = std::chrono::steady_clock::now();
        genStats.update(std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());

        auto tPush0 = std::chrono::steady_clock::now();
        AudioBlock out;
        out.captureNs = std::chrono::duration_cast<std::chrono::nanoseconds>(tPush0.time_since_epoch()).count();
        out.samples = buffer;
        fifo.push(out);
        auto tPush1 = std::chrono::steady_clock::now();
        pushStats.update(std::chrono::duration_cast<std::chrono::nanoseconds>(tPush1 - tPush0).count());

        ++blockCount;
        if ((blockCount.load() & 0xFF) == 0) {
            // periodic report
            std::cout << "[RecorderSim] blocks=" << blockCount.load()
                      << " gen_ns(avg)=" << (genStats.count ? genStats.totalNs / genStats.count : 0)
                      << " gen_max=" << genStats.maxNs
                      << " push_ns(avg)=" << (pushStats.count ? pushStats.totalNs / pushStats.count : 0)
                      << " push_max=" << pushStats.maxNs
                      << std::endl;
        }

        nextWake += blockDuration;
        std::this_thread::sleep_until(nextWake);
    }
}

AudioPlayerSimulator::AudioPlayerSimulator(AudioFifo& f, WCETStats* e2e) : fifo(f), e2eStats(e2e) {}

void AudioPlayerSimulator::start() {
    const auto blockDuration = std::chrono::microseconds(1000000LL * FRAMES_10MS / SAMPLE_RATE);

    while (true) {
        auto t0 = std::chrono::steady_clock::now();
        AudioBlock block = fifo.pop();
        auto t1 = std::chrono::steady_clock::now();
        popStats.update(std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());

        // end-to-end measurement: now - capture timestamp
        if (e2eStats && block.captureNs != 0) {
            uint64_t nowNs = std::chrono::duration_cast<std::chrono::nanoseconds>(t1.time_since_epoch()).count();
            e2eStats->update(nowNs - block.captureNs);
        }

        auto t2 = std::chrono::steady_clock::now();
        (void)block;
        std::this_thread::sleep_for(blockDuration);
        auto t3 = std::chrono::steady_clock::now();
        consumeStats.update(std::chrono::duration_cast<std::chrono::nanoseconds>(t3 - t2).count());

        ++blockCount;
        if ((blockCount.load() & 0xFF) == 0) {
            std::cout << "[PlayerSim] blocks=" << blockCount.load()
                      << " pop_ns(avg)=" << (popStats.count ? popStats.totalNs / popStats.count : 0)
                      << " pop_max=" << popStats.maxNs
                      << " cons_ns(avg)=" << (consumeStats.count ? consumeStats.totalNs / consumeStats.count : 0)
                      << " cons_max=" << consumeStats.maxNs
                      << std::endl;
        }
    }
}
