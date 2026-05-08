#pragma once

#include <queue>
#include <vector>
#include <pthread.h>

/**
 * @class AudioFifo
 * @brief Thread-safe FIFO queue for audio blocks.
 *
 * AudioFifo stores audio buffers as vectors of floating-point samples.
 * It is intended for producer/consumer use, for example one thread recording
 * audio and another thread playing it back.
 */
class AudioFifo {
private:
    /**
     * @brief Queue containing audio blocks.
     */
    std::queue<std::vector<float>> queue;

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
    void push(const std::vector<float>& block);

    /**
     * @brief Removes and returns the oldest audio block from the FIFO.
     *
     * This function blocks until audio data is available.
     *
     * @return The oldest audio block in the queue.
     */
    std::vector<float> pop();
};