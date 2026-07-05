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

AudioPlayer::AudioPlayer(AudioFifo& fifo, WCETStats* e2e) : fifo(fifo), e2eStats(e2e) {}

int AudioPlayer::fillOutput(float* outputBuffer, unsigned long framesPerBuffer) {
    auto procStart = std::chrono::steady_clock::now();

    unsigned long written = 0;
    uint64_t firstCaptureNs = 0;

    // Local jitter buffer. The incoming FIFO contains small blocks
    // e.g. --ms 20 --split 10 gives 10 blocks of 2 ms each.
    // This buffer absorbs small scheduling/network jitter without blocking
    // inside the PortAudio callback.
    static std::deque<AudioBlock> localBuffer;
    static bool started = false;

    // Start after buffering about two complete 20 ms audio buffers.
    // For --split 10 this is 20 small blocks = about 40 ms.
    const size_t START_BLOCKS = static_cast<size_t>(std::max(2 * gBlocksPerPacket, 1));

    // Keep the local buffer bounded so latency cannot grow forever.
    // For --split 10 this is 80 small blocks = about 160 ms.
    const size_t MAX_LOCAL_BLOCKS = static_cast<size_t>(std::max(8 * gBlocksPerPacket, 1));

    // Drain all currently available blocks from the shared FIFO.
    // Use tryPop only: never block inside the PortAudio callback.
    AudioBlock incoming;
    auto drainStart = std::chrono::steady_clock::now();
    while (fifo.tryPop(incoming, false)) {
        localBuffer.push_back(std::move(incoming));

        // If network/FIFO bursts build up too much latency, drop oldest blocks.
        while (localBuffer.size() > MAX_LOCAL_BLOCKS) {
            localBuffer.pop_front();
            gTimingLogger.add("player_hw_jitter_drop", blockCount + 1, 1);
        }
    }
    auto drainEnd = std::chrono::steady_clock::now();

    gTimingLogger.add(
        "player_hw_drain",
        blockCount + 1,
        std::chrono::duration_cast<std::chrono::nanoseconds>(drainEnd - drainStart).count()
    );

    // Warm up without discarding audio. Output silence until enough small
    // blocks are buffered, then start consuming localBuffer.
    if (!started) {
        if (localBuffer.size() < START_BLOCKS) {
            std::memset(outputBuffer, 0, framesPerBuffer * sizeof(float));
            gTimingLogger.add("player_hw_warmup", blockCount + 1, localBuffer.size());
            ++blockCount;
            return 0;
        }
        started = true;
    }

    // Combine small blocks from the local jitter buffer into one full
    // PortAudio output buffer.
    while (written < framesPerBuffer) {
        if (localBuffer.empty()) {
            // Real underrun: not enough received/decoded data available.
            std::memset(
                outputBuffer + written,
                0,
                (framesPerBuffer - written) * sizeof(float)
            );
            gTimingLogger.add("player_hw_underrun", blockCount + 1, framesPerBuffer - written);
            break;
        }

        AudioBlock block = std::move(localBuffer.front());
        localBuffer.pop_front();

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

    auto outputEnd = std::chrono::steady_clock::now();
    gTimingLogger.add(
        "player_hw_proc",
        blockCount + 1,
        std::chrono::duration_cast<std::chrono::nanoseconds>(outputEnd - procStart).count()
    );

    if (e2eStats && firstCaptureNs != 0) {
        auto playbackTime = std::chrono::system_clock::now();
        uint64_t playbackNs = std::chrono::duration_cast<std::chrono::nanoseconds>(
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
