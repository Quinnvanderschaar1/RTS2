#pragma once

#include <queue>
#include <vector>
#include <cstdint>
#include <pthread.h>

/**
 * @class AudioFifo
 * @brief Thread-safe FIFO queue for audio blocks.
 *
 * AudioFifo stores timestamped audio blocks (capture time + samples).
 * It is intended for producer/consumer use, for example one thread recording
 * audio and another thread playing it back.
 */
struct AudioBlock {
    uint64_t captureNs{0};
    std::vector<float> samples;
};

class AudioFifo {
private:
    /**
     * @brief Queue containing audio blocks.
     */
    std::queue<AudioBlock> queue;

    /**
     * @brief Mutex protecting access to the FIFO queue.
     */
    pthread_mutex_t mutex;

    /**
     * @brief Condition variable used to signal when audio data is available.
     */
    pthread_cond_t cond;

public:
    /**
     * @brief Constructs an empty audio FIFO.
     */
    AudioFifo();

    /**
     * @brief Destroys the audio FIFO and releases synchronization resources.
     */
    ~AudioFifo();

    /**
     * @brief Pushes an audio block into the FIFO.
     *
     * @param block Audio block to add to the queue.
     */
    void push(const AudioBlock& block);

    /**
     * @brief Removes and returns the oldest audio block from the FIFO.
     *
     * This function blocks until audio data is available.
     *
     * @return The oldest audio block in the queue.
     */
    AudioBlock pop();
};