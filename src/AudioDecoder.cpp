#include "AudioDecoder.hpp"
#include <cmath>

static inline float muLawToLinear(float x) {
    const float MU = 255.0f;

    float sign = (x < 0) ? -1.0f : 1.0f;
    x = fabs(x);

    float decoded = sign * ((powf(1 + MU, x) - 1) / MU);
    return decoded;
}

AudioBlock AudioDecoder::decode(const AudioBlock& input) {
    AudioBlock out = input;

    for (size_t i = 0; i < input.samples.size(); i++) {
        out.samples[i] = muLawToLinear(input.samples[i]);
    }

    return out;
}