#include "AudioRecorder.hpp"
#include "TimingLogger.hpp"
#include <cstring>
#include <iostream>
#include <vector>
#include <chrono>
#include <pthread.h>
#include <sched.h>
#include <unistd.h>
#include "Globals.hpp"

constexpr int SAMPLE_RATE = 48000;
constexpr int CHANNELS = 1;


static void enableRealtimeThread(int cpu = 0, int priority = 30) {
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

AudioRecorder::AudioRecorder(AudioFifo& fifo) : fifo(fifo) {}

AudioRecorder::AudioRecorder(AudioFifo& fifo, WCETStats* e2e) : fifo(fifo), e2eStats(e2e) {}

int AudioRecorder::processInput(const float* inputBuffer, unsigned long framesPerBuffer) {
    auto sysNow = std::chrono::system_clock::now();
    auto steadyNow = std::chrono::steady_clock::now();
    AudioBlock block;
    block.captureNs = std::chrono::duration_cast<std::chrono::nanoseconds>(
        sysNow.time_since_epoch()).count();
    block.samples.assign(inputBuffer, inputBuffer + framesPerBuffer);
    auto t1 = std::chrono::steady_clock::now();
    uint64_t captureLatency = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - steadyNow).count();
    gTimingLogger.add("recorder_hw_capture", blockCount + 1, captureLatency);

    auto tPush0 = std::chrono::steady_clock::now();
    if (!fifo.tryPush(block, false)) {
        return 0;
    }
    auto tPush1 = std::chrono::steady_clock::now();
    uint64_t pushLatency = std::chrono::duration_cast<std::chrono::nanoseconds>(tPush1 - tPush0).count();
    gTimingLogger.add("recorder_hw_push", blockCount + 1, pushLatency);

    ++blockCount;
    return 0;
}

int AudioRecorder::recordCallback(
    const void* inputBuffer,
    void* outputBuffer,
    unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags,
    void* userData
) {
    (void)outputBuffer;
    (void)timeInfo;
    (void)statusFlags;

    AudioRecorder* recorder = static_cast<AudioRecorder*>(userData);
    const float* in = static_cast<const float*>(inputBuffer);
    if (in == nullptr) {
        static std::vector<float> silence;

        if (silence.size() != static_cast<size_t>(gFramesPerBuffer)) {
            silence.assign(gFramesPerBuffer, 0.0f);
        }
        recorder->processInput(silence.data(), framesPerBuffer);
    } else {
        recorder->processInput(in, framesPerBuffer);
    }
    return paContinue;
}

void AudioRecorder::start() {
    enableRealtimeThread(0, 30);

    Pa_OpenDefaultStream(
        &stream,
        CHANNELS,
        0,
        paFloat32,
        SAMPLE_RATE,
        gFramesPerBuffer,
        &AudioRecorder::recordCallback,
        this
    );

    Pa_StartStream(stream);

    while (true) {
        Pa_Sleep(1000);
    }
}
