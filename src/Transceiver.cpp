#include "Transceiver.hpp"
#include "TimingLogger.hpp"
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
        uint64_t popLatency = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
        stats.pop.update(popLatency);
        gTimingLogger.add("transmit_pop", stats.count + 1, popLatency);

        auto t2 = std::chrono::steady_clock::now();
        bool active = isActive();
        setLed(active);

        if (active) {
            if (sender != nullptr) {
                auto tSend0 = std::chrono::steady_clock::now();
                sender->sendBlock(micBlock);
                auto tSend1 = std::chrono::steady_clock::now();
                uint64_t sendLatency = std::chrono::duration_cast<std::chrono::nanoseconds>(tSend1 - tSend0).count();
                gTimingLogger.add("transmit_send", stats.count + 1, sendLatency);
            } else {
                auto tPush0 = std::chrono::steady_clock::now();
                playbackFifo.push(micBlock);
                auto tPush1 = std::chrono::steady_clock::now();
                uint64_t pushLatency = std::chrono::duration_cast<std::chrono::nanoseconds>(tPush1 - tPush0).count();
                gTimingLogger.add("transmit_local_push", stats.count + 1, pushLatency);
            }
        }
        auto t3 = std::chrono::steady_clock::now();
        uint64_t procLatency = std::chrono::duration_cast<std::chrono::nanoseconds>(t3 - t2).count();
        stats.proc.update(procLatency);
        gTimingLogger.add("transmit_proc", stats.count + 1, procLatency);

        ++stats.count;
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
        uint64_t recvLatency = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();

        if (!remoteBlock.samples.empty()) {
            recvStats.update(recvLatency);
            gTimingLogger.add("receive_recv", receivedCount + 1, recvLatency);

            auto t2 = std::chrono::steady_clock::now();
            playbackFifo.push(remoteBlock);
            auto t3 = std::chrono::steady_clock::now();
            uint64_t pushLatency = std::chrono::duration_cast<std::chrono::nanoseconds>(t3 - t2).count();
            pushStats.update(pushLatency);
            gTimingLogger.add("receive_push", receivedCount + 1, pushLatency);

            ++receivedCount;
        }
    }
}
