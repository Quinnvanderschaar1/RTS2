#include "AudioRecorder.hpp"
#include <vector>
#include <chrono>

constexpr int SAMPLE_RATE = 48000;
constexpr int CHANNELS = 1;
constexpr int FRAMES_10MS = SAMPLE_RATE / 100;

AudioRecorder::AudioRecorder(AudioFifo& fifo) : fifo(fifo) {}

AudioRecorder::AudioRecorder(AudioFifo& fifo, WCETStats* /*e2e*/) : fifo(fifo) {}

void AudioRecorder::start() {
    Pa_OpenDefaultStream(
        &stream,
        CHANNELS,
        0,
        paFloat32,
        SAMPLE_RATE,
        FRAMES_10MS,
        nullptr,
        nullptr
    );

    Pa_StartStream(stream);

    std::vector<float> buffer(FRAMES_10MS * CHANNELS);

    while (true) {
        Pa_ReadStream(stream, buffer.data(), FRAMES_10MS);
        AudioBlock block;
        auto tnow = std::chrono::steady_clock::now();
        block.captureNs = std::chrono::duration_cast<std::chrono::nanoseconds>(tnow.time_since_epoch()).count();
        block.samples = buffer;
        fifo.push(block);
    }
}