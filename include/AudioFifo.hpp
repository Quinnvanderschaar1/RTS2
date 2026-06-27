#pragma once

#include <queue>
#include <vector>
#include <cstdint>
#include <pthread.h>
#include "Globals.hpp"

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
    uint64_t pushNs{0};
    uint64_t sendNs{0};
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

    /**
     * @brief Maximum number of blocks allowed in the FIFO.
     */
    size_t capacity;

    // occupancy tracking
    size_t maxSize{0};
    uint64_t cumulativeSize{0};
    uint64_t occupancyCount{0};

public:
    /**
     * @brief Constructs an empty audio FIFO.
     * @param capacity Maximum number of blocks to buffer.
     */
    AudioFifo(size_t capacity = FIFO_SIZE);

    /**
     * @brief Destroys the audio FIFO and releases synchronization resources.
     */
    ~AudioFifo();

    /**
     * @brief Pushes an audio block into the FIFO.
     *
     * @param block Audio block to add to the queue.
     */
    void push(AudioBlock block);

    /**
     * @brief Removes and returns the oldest audio block from the FIFO.
     *
     * This function blocks until audio data is available.
     *
     * @return The oldest audio block in the queue.
     */
    AudioBlock pop();

    /**
     * @brief Tries to push a block into the FIFO without blocking.
     *
     * Returns false if the FIFO is full or if the lock is unavailable.
     */
    bool tryPush(AudioBlock block, bool log = true);

    /**
     * @brief Tries to pop a block from the FIFO without blocking.
     *
     * Returns false if no block is available or if the lock is unavailable.
     */
    bool tryPop(AudioBlock& block, bool log = true);
};