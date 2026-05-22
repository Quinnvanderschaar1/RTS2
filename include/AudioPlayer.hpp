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
};