#include "SimulatedAudio.hpp"
#include "TimingLogger.hpp"
#include "AudioMixer.hpp"
#include "AudioProcessing.hpp"
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
        uint64_t genLatency = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
        genStats.update(genLatency);
        gTimingLogger.add("recorder_sim_gen", blockCount.load() + 1, genLatency);

        auto tPush0 = std::chrono::steady_clock::now();
        AudioBlock out;
        out.captureNs = std::chrono::duration_cast<std::chrono::nanoseconds>(tPush0.time_since_epoch()).count();
        out.samples = buffer;
        fifo.push(out);
        auto tPush1 = std::chrono::steady_clock::now();
        uint64_t pushLatency = std::chrono::duration_cast<std::chrono::nanoseconds>(tPush1 - tPush0).count();
        pushStats.update(pushLatency);
        gTimingLogger.add("recorder_sim_push", blockCount.load() + 1, pushLatency);

        ++blockCount;

        nextWake += blockDuration;
        auto now = std::chrono::steady_clock::now();
        if (nextWake <= now) {
            nextWake = now + blockDuration;
        } else {
            std::this_thread::sleep_until(nextWake);
        }
    }
}

AudioPlayerSimulator::AudioPlayerSimulator(AudioFifo& f, WCETStats* e2e) : fifo(f), e2eStats(e2e) {}

void AudioPlayerSimulator::start() {
    const auto blockDuration = std::chrono::microseconds(1000000LL * FRAMES_10MS / SAMPLE_RATE);
    AudioMixer mixer;
    AudioProcessing processor;
    auto nextPlayback = std::chrono::steady_clock::now();

    while (true) {
        auto t0 = std::chrono::steady_clock::now();
        AudioBlock block = fifo.pop();
        auto t1 = std::chrono::steady_clock::now();
        uint64_t popLatency = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
        popStats.update(popLatency);
        gTimingLogger.add("player_sim_pop", blockCount.load() + 1, popLatency);

        auto tMix0 = std::chrono::steady_clock::now();
        std::vector<float> mixed = mixer.mix({block.samples});
        auto tMix1 = std::chrono::steady_clock::now();
        gTimingLogger.add("player_sim_mix", blockCount.load() + 1,
            std::chrono::duration_cast<std::chrono::nanoseconds>(tMix1 - tMix0).count());

        auto tProc0 = std::chrono::steady_clock::now();
        std::vector<float> output = processor.lowPass(mixed, 0.5f);
        auto tProc1 = std::chrono::steady_clock::now();
        gTimingLogger.add("player_sim_proc", blockCount.load() + 1,
            std::chrono::duration_cast<std::chrono::nanoseconds>(tProc1 - tProc0).count());

        // keep playback timing independent, but do not hold an extra full cycle if behind
        nextPlayback += blockDuration;
        auto now = std::chrono::steady_clock::now();
        if (nextPlayback > now) {
            std::this_thread::sleep_until(nextPlayback);
        } else {
            nextPlayback = now;
        }

        // playback instant
        auto playbackTime = std::chrono::steady_clock::now();

         // end-to-end measurement: now - capture timestamp
        if (e2eStats && block.captureNs != 0) {
            uint64_t playbackNs = std::chrono::duration_cast<std::chrono::nanoseconds>(playbackTime.time_since_epoch()).count();
            uint64_t latency = playbackNs - block.captureNs;
            e2eStats->update(latency);
            gTimingLogger.add("player_sim_end_to_end", blockCount.load() + 1, latency);
        }

        auto t3 = std::chrono::steady_clock::now();
        uint64_t consumeLatency = std::chrono::duration_cast<std::chrono::nanoseconds>(t3 - t1).count();
        consumeStats.update(consumeLatency);
        gTimingLogger.add("player_sim_consume", blockCount.load() + 1, consumeLatency);

        ++blockCount;
    }
}
