#pragma once

#include <string>
#include <vector>
#include <cstdint>

/**
 * @class UdpSender
 * @brief Sends UDP messages and binary data.
 */
class UdpSender {
private:
    int socketFd = -1;
    std::string address;
    uint16_t port;

public:
    UdpSender(const std::string& address, uint16_t port);
    ~UdpSender();

    void sendMessage(const std::string& message);
    void sendData(const std::vector<float>& data);
};