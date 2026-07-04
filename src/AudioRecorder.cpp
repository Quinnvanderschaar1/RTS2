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

int AudioRecorder::processInput(
    const float* inputBuffer,
    unsigned long framesPerBuffer
) {
    const unsigned long PROCESS_FRAMES =
        static_cast<unsigned long>(gProcessFrames);

    auto sysNow = std::chrono::system_clock::now();
    auto steadyNow = std::chrono::steady_clock::now();

    uint64_t captureNs = std::chrono::duration_cast<std::chrono::nanoseconds>(
        sysNow.time_since_epoch()
    ).count();

    unsigned long pushedBlocks = 0;

    for (unsigned long offset = 0; offset < framesPerBuffer; offset += PROCESS_FRAMES) {
        unsigned long n = std::min(PROCESS_FRAMES, framesPerBuffer - offset);

        AudioBlock block;
        block.captureNs = captureNs;
        block.samples.assign(inputBuffer + offset, inputBuffer + offset + n);

        // Push each smaller processing block immediately so the downstream pipeline
        // can consume it without waiting for the whole callback buffer to be assembled.
        if (!fifo.tryPush(block, false)) {
            break;
        }

        ++pushedBlocks;
    }

    auto t1 = std::chrono::steady_clock::now();
    uint64_t captureLatency =
        std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - steadyNow).count();

    gTimingLogger.add("recorder_hw_capture", blockCount + 1, captureLatency);
    gTimingLogger.add("recorder_hw_push", blockCount + 1, pushedBlocks);

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
    PaError err = Pa_OpenDefaultStream(
        &stream,
        CHANNELS,
        0,
        paFloat32,
        SAMPLE_RATE,
        gFramesPerBuffer,
        &AudioRecorder::recordCallback,
        this
    );

    if (err != paNoError) {
        std::cerr << "Recorder Pa_OpenDefaultStream failed: "
                  << Pa_GetErrorText(err) << std::endl;
        return;
    }

    err = Pa_StartStream(stream);

    if (err != paNoError) {
        std::cerr << "Recorder Pa_StartStream failed: "
                  << Pa_GetErrorText(err) << std::endl;
        Pa_CloseStream(stream);
        return;
    }

    std::cerr << "[RECORDER] stream active = "
              << Pa_IsStreamActive(stream)
              << std::endl;

    // Move realtime priority here
    enableRealtimeThread(0, 30);

    while (true) {
        Pa_Sleep(1000);
    }
}