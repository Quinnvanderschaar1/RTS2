#include "SimulatedAudio.hpp"
#include "TimingLogger.hpp"
#include <chrono>
#include <cmath>
#include <cstring>
#include <thread>
#include <iostream>

#include "Globals.hpp"

#include <algorithm>

constexpr int SAMPLE_RATE = 48000;
constexpr double TWO_PI = 6.28318530717958647692;
constexpr double SIM_TONE_FREQUENCY = 440.0;

AudioRecorderSimulator::AudioRecorderSimulator(AudioFifo& f, WCETStats* e2e) : fifo(f), e2eStats(e2e) {}

void AudioRecorderSimulator::start() {
    const auto fullBufferDuration = std::chrono::microseconds(1000000LL * gFramesPerBuffer / SAMPLE_RATE);
    const auto processBlockFrames = std::max(1, gProcessFrames);
    auto nextWake = std::chrono::steady_clock::now();
    std::vector<float> buffer(gFramesPerBuffer);

    while (true) {
        auto t0 = std::chrono::steady_clock::now();
        for (int i = 0; i < gFramesPerBuffer; ++i) {
            buffer[i] = 0.5f * static_cast<float>(std::sin(TWO_PI * SIM_TONE_FREQUENCY * phase / SAMPLE_RATE));
            phase += 1.0;
            if (phase >= SAMPLE_RATE) phase -= SAMPLE_RATE;
        }
        auto t1 = std::chrono::steady_clock::now();
        uint64_t genLatency = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
        genStats.update(genLatency);
        gTimingLogger.add("recorder_sim_gen", blockCount.load() + 1, genLatency);

        auto sysPush0 = std::chrono::system_clock::now();
        const uint64_t captureNs = std::chrono::duration_cast<std::chrono::nanoseconds>(sysPush0.time_since_epoch()).count();

        for (int offset = 0; offset < gFramesPerBuffer; offset += processBlockFrames) {
            const int n = std::min(processBlockFrames, gFramesPerBuffer - offset);
            auto steadyPush0 = std::chrono::steady_clock::now();
            AudioBlock out;
            out.captureNs = captureNs;
            out.samples.assign(buffer.begin() + offset, buffer.begin() + offset + n);
            bool pushed = fifo.tryPush(out, false);
            if (!pushed) {
                gTimingLogger.addDropSample({captureNs, "mic", "recorder_sim"});
            }
            auto tPush1 = std::chrono::steady_clock::now();
            uint64_t pushLatency = std::chrono::duration_cast<std::chrono::nanoseconds>(tPush1 - steadyPush0).count();
            if (pushed) {
                pushStats.update(pushLatency);
                gTimingLogger.add("recorder_sim_push", blockCount.load() + 1, pushLatency);
                ++blockCount;
            } else {
                break;
            }
        }

        nextWake += fullBufferDuration;
        auto now = std::chrono::steady_clock::now();
        if (nextWake <= now) {
            nextWake = now + fullBufferDuration;
        } else {
            std::this_thread::sleep_until(nextWake);
        }
    }
}

AudioPlayerSimulator::AudioPlayerSimulator(AudioFifo& f, WCETStats* e2e) : fifo(f), e2eStats(e2e) {}

void AudioPlayerSimulator::start() {
    const auto blockDuration = std::chrono::microseconds(1000000LL * std::max(1, gFramesPerBuffer) / SAMPLE_RATE);
    auto nextPlayback = std::chrono::steady_clock::now();

    while (true) {
        const unsigned long framesPerBuffer = static_cast<unsigned long>(std::max(1, gFramesPerBuffer));
        std::vector<float> output(framesPerBuffer, 0.0f);
        unsigned long written = 0;
        uint64_t firstCaptureNs = 0;

        auto t0 = std::chrono::steady_clock::now();
        while (written < framesPerBuffer) {
            AudioBlock block;
            auto tPop0 = std::chrono::steady_clock::now();
            bool popped = fifo.tryPop(block, false);
            auto tPop1 = std::chrono::steady_clock::now();
            uint64_t popLatency = std::chrono::duration_cast<std::chrono::nanoseconds>(tPop1 - tPop0).count();
            popStats.update(popLatency);
            gTimingLogger.add("player_sim_pop", blockCount.load() + 1, popLatency);

            if (!popped) {
                std::memset(output.data() + written, 0, (framesPerBuffer - written) * sizeof(float));
                written = framesPerBuffer;
                break;
            }

            if (firstCaptureNs == 0 && block.captureNs != 0) {
                firstCaptureNs = block.captureNs;
            }

            const unsigned long n = std::min<unsigned long>(block.samples.size(), framesPerBuffer - written);
            if (n > 0) {
                std::copy(block.samples.begin(), block.samples.begin() + n, output.begin() + written);
                written += n;
            }
        }
        auto t1 = std::chrono::steady_clock::now();
        uint64_t consumeLatency = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
        consumeStats.update(consumeLatency);
        gTimingLogger.add("player_sim_consume", blockCount.load() + 1, consumeLatency);

        // keep playback timing independent, but do not hold an extra full cycle if behind
        nextPlayback += blockDuration;
        auto now = std::chrono::steady_clock::now();
        if (nextPlayback > now) {
            std::this_thread::sleep_until(nextPlayback);
        } else {
            nextPlayback = now;
        }

        // playback instant
        auto playbackTime = std::chrono::system_clock::now();

        // end-to-end measurement: now - capture timestamp
        if (e2eStats && firstCaptureNs != 0) {
            uint64_t playbackNs = std::chrono::duration_cast<std::chrono::nanoseconds>(playbackTime.time_since_epoch()).count();
            uint64_t latency = playbackNs - firstCaptureNs;
            e2eStats->update(latency);
            gTimingLogger.add("player_sim_end_to_end", blockCount.load() + 1, latency);
        }

        ++blockCount;
    }
}
