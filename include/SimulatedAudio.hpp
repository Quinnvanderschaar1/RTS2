#pragma once

#include "AudioFifo.hpp"
#include "wcet.hpp"

#include <atomic>
#include <vector>

/**
 * @brief Simulates an audio recorder.
 *
 * Generates synthetic audio samples at the configured sample rate and pushes
 * them into the audio FIFO at the configured block interval.
 */
class AudioRecorderSimulator {
private:
    AudioFifo& fifo;                      ///< Output FIFO for generated audio blocks.
    double phase = 0.0;                  ///< Phase accumulator for the generated sine wave.
    std::atomic<uint64_t> blockCount{0}; ///< Number of generated audio blocks.
    WCETStats genStats;                  ///< Timing statistics for audio generation.
    WCETStats pushStats;                 ///< Timing statistics for FIFO pushes.
    WCETStats* e2eStats{nullptr};        ///< Optional end-to-end latency statistics.

public:
    /**
     * @brief Constructs a simulated audio recorder.
     *
     * @param fifo FIFO receiving generated audio blocks.
     * @param e2eStats Optional end-to-end latency statistics collector.
     */
    explicit AudioRecorderSimulator(AudioFifo& fifo, WCETStats* e2eStats = nullptr);

    /**
     * @brief Starts the simulated recorder.
     *
     * Continuously generates audio blocks and pushes them into the FIFO.
     */
    void start();
};

/**
 * @brief Simulates an audio player.
 *
 * Consumes audio blocks from a FIFO at the configured playback interval and
 * performs simulated playback while collecting timing statistics.
 */
class AudioPlayerSimulator {
private:
    AudioFifo& fifo;                      ///< Input FIFO containing audio blocks.
    std::atomic<uint64_t> blockCount{0}; ///< Number of consumed audio blocks.
    WCETStats popStats;                  ///< Timing statistics for FIFO pops.
    WCETStats consumeStats;              ///< Timing statistics for simulated playback.
    WCETStats* e2eStats{nullptr};        ///< Optional end-to-end latency statistics.

public:
    /**
     * @brief Constructs a simulated audio player.
     *
     * @param fifo FIFO supplying audio blocks.
     * @param e2eStats Optional end-to-end latency statistics collector.
     */
    explicit AudioPlayerSimulator(AudioFifo& fifo, WCETStats* e2eStats = nullptr);

    /**
     * @brief Starts the simulated player.
     *
     * Continuously consumes audio blocks from the FIFO and simulates playback.
     */
    void start();
};