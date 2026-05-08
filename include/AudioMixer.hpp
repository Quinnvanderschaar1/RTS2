#pragma once

#include <vector>
#include <cstddef>

/**
 * @class AudioMixer
 * @brief Mixes several audio input signals into one output signal.
 */
class AudioMixer {
public:
    /**
     * @brief Mix multiple audio buffers into one buffer.
     *
     * All input buffers should contain float samples in range [-1.0, 1.0].
     * The output is averaged to avoid clipping.
     *
     * @param inputs List of input audio buffers.
     * @return Mixed output buffer.
     */
    std::vector<float> mix(const std::vector<std::vector<float>>& inputs) const;
};