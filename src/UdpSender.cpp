#include "UdpSender.hpp"

#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <endian.h>
#include <chrono>

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

    // Payload: first 8 bytes = captureNs, next 8 bytes = sendNs, both network byte order, followed by float samples
    uint64_t netCapture = htobe64(block.captureNs);
    uint64_t sendNs = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    uint64_t netSend = htobe64(sendNs);

    size_t payloadSize = sizeof(netCapture) + sizeof(netSend) + block.samples.size() * sizeof(float);
    std::vector<char> payload(payloadSize);
    memcpy(payload.data(), &netCapture, sizeof(netCapture));
    memcpy(payload.data() + sizeof(netCapture), &netSend, sizeof(netSend));
    memcpy(payload.data() + sizeof(netCapture) + sizeof(netSend), block.samples.data(), block.samples.size() * sizeof(float));

    sendto(
        socketFd,
        payload.data(),
        payload.size(),
        0,
        reinterpret_cast<sockaddr*>(&addr),
        sizeof(addr)
    );
}