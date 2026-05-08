#include "AudioRecorder.hpp"
#include <vector>

constexpr int SAMPLE_RATE = 48000;
constexpr int CHANNELS = 1;
constexpr int FRAMES_10MS = SAMPLE_RATE / 100;

AudioRecorder::AudioRecorder(AudioFifo& fifo) : fifo(fifo) {}

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
        fifo.push(buffer);
    }
}