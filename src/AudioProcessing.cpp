#include "AudioProcessing.hpp"
#include <cstddef>

std::vector<float> AudioProcessing::lowPass(
    const std::vector<float>& input,
    float alpha
) {
    if (input.empty()) {
        return {};
    }

    std::vector<float> output(input.size());
    output[0] = input[0];

    for (size_t i = 1; i < input.size(); ++i) {
        output[i] = alpha * input[i] + (1.0f - alpha) * output[i - 1];
    }

    return output;
}

std::vector<float> AudioProcessing::echoCancellation(
    const std::vector<float>& input,
    int delaySamples,
    float decay
) {
    std::vector<float> output(input.size());

    for (size_t i = 0; i < input.size(); ++i) {
        float echo = 0.0f;

        if (static_cast<int>(i) >= delaySamples) {
            echo = output[i - delaySamples] * decay;
        }

        output[i] = input[i] - echo;

        if (output[i] > 1.0f) output[i] = 1.0f;
        if (output[i] < -1.0f) output[i] = -1.0f;
    }

    return output;
}