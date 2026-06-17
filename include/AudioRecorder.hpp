#pragma once

#include "AudioFifo.hpp"
#include "wcet.hpp"
#include <portaudio.h>

/**
 * @class AudioRecorder
 * @brief Audio recording class using PortAudio.
 *
 * AudioRecorder continuously captures audio from the default
 * input device and pushes 10 ms audio blocks into an AudioFifo.
 */
class AudioRecorder {
private:
    /**
     * @brief Reference to the shared audio FIFO.
     */
    AudioFifo& fifo;

    /**
     * @brief PortAudio input stream.
     */
    PaStream* stream = nullptr;

    /**
     * @brief Optional end-to-end statistics collector.
     */
    WCETStats* e2eStats = nullptr;

    /**
     * @brief Count of audio blocks processed.
     */
    uint64_t blockCount = 0;

public:
    /**
     * @brief Constructs an AudioRecorder.
     *
     * @param fifo Reference to the audio FIFO used to store recorded audio.
     */
    explicit AudioRecorder(AudioFifo& fifo);

    explicit AudioRecorder(AudioFifo& fifo, WCETStats* e2eStats);

    /**
     * @brief Starts the recording loop.
     *
     * Opens the PortAudio input stream and continuously records
     * audio blocks into the FIFO.
     */
    void start();

private:
    static int recordCallback(
        const void* inputBuffer,
        void* outputBuffer,
        unsigned long framesPerBuffer,
        const PaStreamCallbackTimeInfo* timeInfo,
        PaStreamCallbackFlags statusFlags,
        void* userData
    );
    int processInput(const float* inputBuffer, unsigned long framesPerBuffer);
};