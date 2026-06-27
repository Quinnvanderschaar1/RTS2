#pragma once

#include <vector>

/**
 * @class AudioProcessing
 * @brief Provides basic audio processing functions.
 */
class AudioProcessing {
public:
    /**
     * @brief Applies a simple first-order low-pass filter.
     *
     * @param input Input audio samples.
     * @param alpha Filter smoothing factor between 0.0 and 1.0.
     *              Lower values create stronger smoothing.
     * @return Filtered audio samples.
     */
    std::vector<float> lowPass(
        const std::vector<float>& input,
        float alpha
    );

    /**
     * @brief Applies simple echo cancellation.
     *
     * This subtracts a delayed and scaled version of the signal.
     *
     * @param input Input audio samples.
     * @param delaySamples Echo delay in samples.
     * @param decay Echo reduction factor.
     * @return Echo-reduced audio samples.
     */
    std::vector<float> echoCancellation(
        const std::vector<float>& input,
        int delaySamples,
        float decay
    );

    /**
     * @brief Applies simple audio encoding.
     *
     * @param input Input audio samples.
     * @return Encoded audio samples.
     */
    std::vector<float> audioEncoding(
        const std::vector<float>& input
    );

    /**
     * @brief Applies simple audio decoding.
     *
     * @param input Input audio samples.
     * @return Decoded audio samples.
     */
    std::vector<float> audioDecoding(
        const std::vector<float>& input
    );

     /**
     * @brief Applies simple heavy loop processing.
     *
     * @param input Input audio samples.
     */
    static void HeavyLoop(int loop_size
    );
};