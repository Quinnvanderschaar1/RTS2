#include "UdpReceiver.hpp"

#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <endian.h>

UdpReceiver::UdpReceiver(const std::string& address, uint16_t port)
    : address(address), port(port)
{
    socketFd = socket(AF_INET, SOCK_DGRAM, 0);

    if (socketFd < 0) {
        perror("socket");
        return;
    }

    int reuse = 1;
    setsockopt(socketFd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    sockaddr_in localAddr{};
    localAddr.sin_family = AF_INET;
    localAddr.sin_port = htons(port);
    localAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(socketFd, reinterpret_cast<sockaddr*>(&localAddr), sizeof(localAddr)) < 0) {
        perror("bind");
        close(socketFd);
        socketFd = -1;
        return;
    }

    // Join multicast group only if the address is in the multicast range (224.0.0.0/4).
    in_addr_t addr = inet_addr(address.c_str());
    uint32_t hostAddr = ntohl(addr);
    if ((hostAddr & 0xF0000000) == 0xE0000000) {
        ip_mreq mreq{};
        mreq.imr_multiaddr.s_addr = addr;
        mreq.imr_interface.s_addr = htonl(INADDR_ANY);
        setsockopt(socketFd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));
    }
}

UdpReceiver::~UdpReceiver() {
    if (socketFd >= 0) {
        close(socketFd);
    }
}

std::string UdpReceiver::receiveMessage(size_t maxSize) {
    std::vector<char> buffer(maxSize);

    ssize_t bytes = recvfrom(
        socketFd,
        buffer.data(),
        buffer.size(),
        0,
        nullptr,
        nullptr
    );

    if (bytes <= 0) {
        return "";
    }

    return std::string(buffer.data(), bytes);
}

std::vector<float> UdpReceiver::receiveFloatData(size_t maxFloats) {
    std::vector<float> buffer(maxFloats);

    ssize_t bytes = recvfrom(
        socketFd,
        buffer.data(),
        buffer.size() * sizeof(float),
        0,
        nullptr,
        nullptr
    );

    if (bytes <= 0) {
        return {};
    }

    buffer.resize(bytes / sizeof(float));
    return buffer;
}

AudioBlock UdpReceiver::receiveBlock(size_t maxFloats) {
    // Buffer large enough for timestamp + floats
    size_t bufBytes = sizeof(uint64_t) + maxFloats * sizeof(float);
    std::vector<char> buffer(bufBytes);

    ssize_t bytes = recvfrom(
        socketFd,
        buffer.data(),
        buffer.size(),
        0,
        nullptr,
        nullptr
    );

    if (bytes <= 0) {
        return {};
    }

    if ((size_t)bytes < sizeof(uint64_t)) return {};

    uint64_t netTs;
    memcpy(&netTs, buffer.data(), sizeof(netTs));
    uint64_t ts = be64toh(netTs);

    size_t floatsBytes = bytes - sizeof(uint64_t);
    size_t nFloats = floatsBytes / sizeof(float);
    AudioBlock block;
    block.captureNs = ts;
    block.samples.resize(nFloats);
    memcpy(block.samples.data(), buffer.data() + sizeof(uint64_t), nFloats * sizeof(float));
    return block;
}