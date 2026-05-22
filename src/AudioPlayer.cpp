#include "AudioPlayer.hpp"

constexpr int SAMPLE_RATE = 48000;
constexpr int CHANNELS = 1;
constexpr int FRAMES_10MS = SAMPLE_RATE / 100;


AudioPlayer::AudioPlayer(AudioFifo& fifo) : fifo(fifo) {}

AudioPlayer::AudioPlayer(AudioFifo& fifo, WCETStats* /*e2e*/) : fifo(fifo) {}

void AudioPlayer::start() {
    Pa_OpenDefaultStream(
        &stream,
        0,
        CHANNELS,
        paFloat32,
        SAMPLE_RATE,
        FRAMES_10MS,
        nullptr,
        nullptr
    );

    Pa_StartStream(stream);

    while (true) {
        AudioBlock block = fifo.pop();
        Pa_WriteStream(stream, block.samples.data(), FRAMES_10MS);
    }
}