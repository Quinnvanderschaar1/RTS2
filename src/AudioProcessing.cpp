#include "AudioProcessing.hpp"
#include <cstddef>
#include <cmath>

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

std::vector<float> AudioProcessing::audioEncoding(
    const std::vector<float>& input
) {
    std::vector<float> output(input.size());
    const float MU = 255.0f;
    for (size_t i = 0; i < input.size(); i++) {
        float sign = (input[i] < 0) ? -1.0f : 1.0f;
        float x = fabs(input[i]);

        float encoded = sign * (logf(1 + MU * x) / logf(1 + MU));
        output[i] = encoded;
    }
    return output;
}

std::vector<float> AudioProcessing::audioDecoding(
    const std::vector<float>& input
) {
    const float MU = 255.0f;
    std::vector<float> output(input.size());
    
    for (size_t i = 0; i < input.size(); i++) {
    float sign = (input[i] < 0) ? -1.0f : 1.0f;
    float x = fabs(input[i]);

    float decoded = sign * ((powf(1 + MU, x) - 1) / MU);
     output[i] = decoded;
    }
    return output;
}

void AudioProcessing::HeavyLoop(int loop_size=0
) {
    // Simulate a heavy processing loop
    volatile double sum = 0.0;
    for (int i = 0; i < loop_size; ++i) {
        sum += std::sin(i) * std::cos(i);
    }
}