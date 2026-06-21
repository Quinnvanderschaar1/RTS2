#include "AudioEncoder.hpp"
#include <cmath>

static inline float linearToMuLaw(float x) {
    const float MU = 255.0f;
    float sign = (x < 0) ? -1.0f : 1.0f;
    x = fabs(x);

    float encoded = sign * (logf(1 + MU * x) / logf(1 + MU));
    return encoded;
}

AudioBlock AudioEncoder::encode(const AudioBlock& input) {
    AudioBlock out = input;

    for (size_t i = 0; i < input.samples.size(); i++) {
        out.samples[i] = linearToMuLaw(input.samples[i]);
    }

    return out;
}