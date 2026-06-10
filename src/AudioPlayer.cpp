#include "AudioPlayer.hpp"
#include <cstring>
#include <iostream>
#include <pthread.h>
#include <sched.h>
#include <unistd.h>

constexpr int SAMPLE_RATE = 48000;
constexpr int CHANNELS = 1;
constexpr int FRAMES_10MS = SAMPLE_RATE / 100;

static void enableRealtimeThread(int cpu = 1, int priority = 29) {
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

AudioPlayer::AudioPlayer(AudioFifo& fifo, WCETStats* /*e2e*/) : fifo(fifo) {}

int AudioPlayer::fillOutput(float* outputBuffer, unsigned long framesPerBuffer) {
    AudioBlock block;
    if (!fifo.tryPop(block, false)) {
        std::memset(outputBuffer, 0, framesPerBuffer * sizeof(float));
        return 0;
    }

    std::memcpy(outputBuffer, block.samples.data(), framesPerBuffer * sizeof(float));
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
        FRAMES_10MS,
        &AudioPlayer::playCallback,
        this
    );

    Pa_StartStream(stream);

    while (true) {
        Pa_Sleep(1000);
    }
}
