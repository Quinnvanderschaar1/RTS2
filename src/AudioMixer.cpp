#include "AudioMixer.hpp"

#include <algorithm>

std::vector<float> AudioMixer::mix(
    const std::vector<std::vector<float>>& inputs
) const {
    if (inputs.empty()) {
        return {};
    }

    size_t maxSize = 0;

    for (const auto& input : inputs) {
        maxSize = std::max(maxSize, input.size());
    }

    std::vector<float> output(maxSize, 0.0f);

    for (const auto& input : inputs) {
        for (size_t i = 0; i < input.size(); ++i) {
            output[i] += input[i];
        }
    }

    float scale = 1.0f / static_cast<float>(inputs.size());

    for (auto& sample : output) {
        sample *= scale;

        if (sample > 1.0f) sample = 1.0f;
        if (sample < -1.0f) sample = -1.0f;
    }

    return output;
}