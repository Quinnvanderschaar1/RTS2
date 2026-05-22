#include "Transceiver.hpp"
#include <chrono>
#include <iostream>

void transmitLoop(
    AudioFifo& micFifo,
    AudioFifo& playbackFifo,
    const std::function<bool()>& isActive,
    const std::function<void(bool)>& setLed,
    UdpSender* sender,
    bool simulationMode
) {
    struct LocalStats { WCETStats pop; WCETStats proc; uint64_t count{0}; } stats;

    while (true) {
        auto t0 = std::chrono::steady_clock::now();
        AudioBlock micBlock = micFifo.pop();
        auto t1 = std::chrono::steady_clock::now();
        stats.pop.update(std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());

        auto t2 = std::chrono::steady_clock::now();
        bool active = isActive();
        setLed(active);

        if (active) {
            if (sender != nullptr) {
                // send via UDP (includes timestamp)
                sender->sendBlock(micBlock);
            } else {
                // local loop: push directly to playback
                playbackFifo.push(micBlock);
            }
        }
        auto t3 = std::chrono::steady_clock::now();
        stats.proc.update(std::chrono::duration_cast<std::chrono::nanoseconds>(t3 - t2).count());

        if ((++stats.count & 0xFF) == 0) {
            std::cout << "[Transmit] blocks=" << stats.count
                      << " pop_ns(avg)=" << (stats.pop.count ? stats.pop.totalNs / stats.pop.count : 0)
                      << " pop_max=" << stats.pop.maxNs
                      << " proc_ns(avg)=" << (stats.proc.count ? stats.proc.totalNs / stats.proc.count : 0)
                      << " proc_max=" << stats.proc.maxNs
                      << std::endl;
        }
    }
}

void receiveLoop(AudioFifo& playbackFifo, UdpReceiver& receiver) {
    uint64_t receivedCount = 0;
    WCETStats recvStats;
    WCETStats pushStats;

    while (true) {
        auto t0 = std::chrono::steady_clock::now();
        AudioBlock remoteBlock = receiver.receiveBlock(480);
        auto t1 = std::chrono::steady_clock::now();
        recvStats.update(std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());

        if (!remoteBlock.samples.empty()) {
            auto t2 = std::chrono::steady_clock::now();
            playbackFifo.push(remoteBlock);
            auto t3 = std::chrono::steady_clock::now();
            pushStats.update(std::chrono::duration_cast<std::chrono::nanoseconds>(t3 - t2).count());

            if ((++receivedCount & 0xFF) == 0) {
                std::cout << "[Receive] packets=" << receivedCount
                          << " recv_ns(avg)=" << (recvStats.count ? recvStats.totalNs / recvStats.count : 0)
                          << " recv_max=" << recvStats.maxNs
                          << " push_ns(avg)=" << (pushStats.count ? pushStats.totalNs / pushStats.count : 0)
                          << " push_max=" << pushStats.maxNs
                          << std::endl;
            }
        }
    }
}
