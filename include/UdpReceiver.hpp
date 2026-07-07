#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include "AudioFifo.hpp"

/**
 * @class UdpReceiver
 * @brief Receives UDP messages and binary data.
 */
class UdpReceiver {
private:
    int socketFd = -1;
    std::string address;
    uint16_t port;

public:
    UdpReceiver(const std::string& address, uint16_t port);
    ~UdpReceiver();

    std::string receiveMessage(size_t maxSize = 4096);
    std::vector<float> receiveFloatData(size_t maxFloats);
    AudioBlock receiveBlock(size_t maxFloats);
};