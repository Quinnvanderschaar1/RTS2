#include "AudioBlock.hpp"
#include <vector>

class AudioEncoder {
public:
    AudioBlock encode(const AudioBlock& input);
};