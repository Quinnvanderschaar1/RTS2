#include "AudioPlayer.hpp"
#include "TimingLogger.hpp"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <deque>
#include <iostream>
#include <pthread.h>
#include <sched.h>
#include <unistd.h>

#include "Globals.hpp"

constexpr int SAMPLE_RATE = 48000;
constexpr int CHANNELS = 1;

static void enableRealtimeThread(int cpu = 1, int priority = 70) {
#if defined(__linux__)
    cpu_set_t cpus;
    CPU_ZERO(&cpus);
    CPU_SET(cpu, &cpus);

    pthread_t thread = pthread_self();

    if (pthread_setaffinity_np(thread, sizeof(cpus), &cpus) != 0) {
        perror("pthread_setaffinity_np");
    }

    sched_param param{};
    param.sched_priority = priority;

    if (pthread_setschedparam(thread, SCHED_FIFO, &param) != 0) {
        perror("pthread_setschedparam");
    }
#endif
}

AudioPlayer::AudioPlayer(AudioFifo& fifo) : fifo(fifo) {}

AudioPlayer::AudioPlayer(AudioFifo& fifo, WCETStats* e2e)
    : fifo(fifo), e2eStats(e2e) {}

int AudioPlayer::fillOutput(float* outputBuffer, unsigned long framesPerBuffer) {
    auto procStart = std::chrono::steady_clock::now();

    static std::deque<AudioBlock> localBuffer;
    static bool started = false;

    unsigned long written = 0;
    uint64_t firstCaptureNs = 0;

    const size_t START_BLOCKS =
        static_cast<size_t>(std::max(3 * gBlocksPerPacket, 1));

    const size_t MAX_LOCAL_BLOCKS =
        static_cast<size_t>(std::max(12 * gBlocksPerPacket, 1));

    size_t drained = 0;
    AudioBlock incoming;

    while (fifo.tryPop(incoming, false)) {
        localBuffer.push_back(std::move(incoming));
        ++drained;

        while (localBuffer.size() > MAX_LOCAL_BLOCKS) {
            localBuffer.pop_front();
            gTimingLogger.add("player_hw_jitter_drop", blockCount + 1, 1);
        }
    }

    if (!started) {
        if (localBuffer.size() < START_BLOCKS) {
            std::memset(outputBuffer, 0, framesPerBuffer * sizeof(float));
            gTimingLogger.add("player_hw_warmup", blockCount + 1, localBuffer.size());
            ++blockCount;
            return 0;
        }

        started = true;
    }

    size_t blocksUsed = 0;

    while (written < framesPerBuffer) {
        if (localBuffer.empty()) {
            std::memset(
                outputBuffer + written,
                0,
                (framesPerBuffer - written) * sizeof(float)
            );

            gTimingLogger.add(
                "player_hw_underrun",
                blockCount + 1,
                framesPerBuffer - written
            );

            break;
        }

        AudioBlock block = std::move(localBuffer.front());
        localBuffer.pop_front();
        ++blocksUsed;

        if (firstCaptureNs == 0 && block.captureNs != 0) {
            firstCaptureNs = block.captureNs;
        }

        unsigned long n = std::min<unsigned long>(
            block.samples.size(),
            framesPerBuffer - written
        );

        if (n > 0) {
            std::memcpy(
                outputBuffer + written,
                block.samples.data(),
                n * sizeof(float)
            );

            written += n;
        }
    }

    auto procEnd = std::chrono::steady_clock::now();

    gTimingLogger.add(
        "player_hw_proc",
        blockCount + 1,
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            procEnd - procStart
        ).count()
    );

    gTimingLogger.add("player_hw_drained_blocks", blockCount + 1, drained);
    gTimingLogger.add("player_hw_used_blocks", blockCount + 1, blocksUsed);
    gTimingLogger.add("player_hw_remaining_blocks", blockCount + 1, localBuffer.size());

    if (e2eStats && firstCaptureNs != 0) {
        auto playbackTime = std::chrono::system_clock::now();

        uint64_t playbackNs =
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                playbackTime.time_since_epoch()
            ).count();

        uint64_t latency = playbackNs - firstCaptureNs;

        e2eStats->update(latency);
        gTimingLogger.add("player_hw_end_to_end", blockCount + 1, latency);
    }

    ++blockCount;
    return 0;
}

int AudioPlayer::playCallback(
    const void* inputBuffer,
    void* outputBuffer,
    unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags,
    void* userData
) {
    (void)inputBuffer;
    (void)timeInfo;
    (void)statusFlags;

    AudioPlayer* player = static_cast<AudioPlayer*>(userData);
    player->fillOutput(static_cast<float*>(outputBuffer), framesPerBuffer);

    return paContinue;
}

void AudioPlayer::start() {
    enableRealtimeThread(1, 29);

    PaError err = Pa_OpenDefaultStream(
        &stream,
        0,
        CHANNELS,
        paFloat32,
        SAMPLE_RATE,
        gFramesPerBuffer,
        &AudioPlayer::playCallback,
        this
    );

    if (err != paNoError) {
        std::cerr << "Player Pa_OpenDefaultStream failed: "
                  << Pa_GetErrorText(err) << std::endl;
        return;
    }

    err = Pa_StartStream(stream);

    if (err != paNoError) {
        std::cerr << "Player Pa_StartStream failed: "
                  << Pa_GetErrorText(err) << std::endl;
        Pa_CloseStream(stream);
        return;
    }

    while (true) {
        Pa_Sleep(1000);
    }
}
