#include "UdpSender.hpp"

#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <endian.h>

UdpSender::UdpSender(const std::string& address, uint16_t port)
    : address(address), port(port)
{
    socketFd = socket(AF_INET, SOCK_DGRAM, 0);

    if (socketFd < 0) {
        perror("socket");
        return;
    }

    unsigned char ttl = 1;
    setsockopt(socketFd, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl));
}

UdpSender::~UdpSender() {
    if (socketFd >= 0) {
        close(socketFd);
    }
}

void UdpSender::sendMessage(const std::string& message) {
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, address.c_str(), &addr.sin_addr);

    sendto(
        socketFd,
        message.data(),
        message.size(),
        0,
        reinterpret_cast<sockaddr*>(&addr),
        sizeof(addr)
    );
}

void UdpSender::sendData(const std::vector<float>& data) {
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, address.c_str(), &addr.sin_addr);

    sendto(
        socketFd,
        data.data(),
        data.size() * sizeof(float),
        0,
        reinterpret_cast<sockaddr*>(&addr),
        sizeof(addr)
    );
}

void UdpSender::sendBlock(const AudioBlock& block) {
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, address.c_str(), &addr.sin_addr);

    // Payload: first 8 bytes = captureNs (uint64_t, network byte order), followed by float samples
    uint64_t netTs = htobe64(block.captureNs);
    size_t payloadSize = sizeof(netTs) + block.samples.size() * sizeof(float);
    std::vector<char> payload(payloadSize);
    memcpy(payload.data(), &netTs, sizeof(netTs));
    memcpy(payload.data() + sizeof(netTs), block.samples.data(), block.samples.size() * sizeof(float));

    sendto(
        socketFd,
        payload.data(),
        payload.size(),
        0,
        reinterpret_cast<sockaddr*>(&addr),
        sizeof(addr)
    );
}