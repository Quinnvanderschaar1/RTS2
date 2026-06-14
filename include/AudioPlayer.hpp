#pragma once

#include "AudioFifo.hpp"
#include "wcet.hpp"
#include <portaudio.h>

/**
 * @class AudioPlayer
 * @brief Audio playback class using PortAudio.
 *
 * AudioPlayer continuously retrieves audio blocks from an AudioFifo
 * instance and plays them back through the default audio output device.
 */
class AudioPlayer {
private:
    /**
     * @brief Reference to the shared audio FIFO.
     */
    AudioFifo& fifo;

    /**
     * @brief PortAudio playback stream.
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
     * @brief Constructs an AudioPlayer.
     *
     * @param fifo Reference to the audio FIFO used for playback.
     */
    explicit AudioPlayer(AudioFifo& fifo);

    explicit AudioPlayer(AudioFifo& fifo, WCETStats* e2eStats);

    /**
     * @brief Starts the playback loop.
     *
     * Opens the PortAudio output stream and continuously plays
     * audio blocks received from the FIFO.
     */
    void start();

private:
    static int playCallback(
        const void* inputBuffer,
        void* outputBuffer,
        unsigned long framesPerBuffer,
        const PaStreamCallbackTimeInfo* timeInfo,
        PaStreamCallbackFlags statusFlags,
        void* userData
    );
    int fillOutput(float* outputBuffer, unsigned long framesPerBuffer);
};