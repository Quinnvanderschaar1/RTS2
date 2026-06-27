#include "AudioPlayer.hpp"
#include "TimingLogger.hpp"
#include <chrono>
#include <cstring>
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

AudioPlayer::AudioPlayer(AudioFifo& fifo, WCETStats* e2e) : fifo(fifo), e2eStats(e2e) {}

int AudioPlayer::fillOutput(float* outputBuffer, unsigned long framesPerBuffer) {
    auto tPop0 = std::chrono::steady_clock::now();
    AudioBlock block;
    if (!fifo.tryPop(block, false)) {
        std::memset(outputBuffer, 0, framesPerBuffer * sizeof(float));
        return 0;
    }
    auto tPop1 = std::chrono::steady_clock::now();
    uint64_t popLatency = std::chrono::duration_cast<std::chrono::nanoseconds>(tPop1 - tPop0).count();
    gTimingLogger.add("player_hw_pop", blockCount + 1, popLatency);

    auto tCopy0 = std::chrono::steady_clock::now();
    std::memcpy(outputBuffer, block.samples.data(), framesPerBuffer * sizeof(float));
    auto tCopy1 = std::chrono::steady_clock::now();
    uint64_t outputLatency = std::chrono::duration_cast<std::chrono::nanoseconds>(tCopy1 - tCopy0).count();
    gTimingLogger.add("player_hw_output", blockCount + 1, outputLatency);

    if (e2eStats) {
        uint64_t playbackNs = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        if (block.sendNs != 0) {
            uint64_t latency = playbackNs - block.sendNs;
            e2eStats->update(latency);
            //gTimingLogger.add("player_hw_end_to_end", blockCount + 1, latency);
        } else if (block.captureNs != 0) {
            uint64_t latency = playbackNs - block.captureNs;
            e2eStats->update(latency);
            //gTimingLogger.add("player_local_end_to_end", blockCount + 1, latency);
        }
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

    Pa_OpenDefaultStream(
        &stream,
        0,
        CHANNELS,
        paFloat32,
        SAMPLE_RATE,
        gFramesPerBuffer,
        &AudioPlayer::playCallback,
        this
    );

    Pa_StartStream(stream);

    while (true) {
        Pa_Sleep(1000);
    }
}
