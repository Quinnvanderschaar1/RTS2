#include "Transceiver.hpp"
#include "TimingLogger.hpp"
#include <chrono>
#include <iostream>

#include "Globals.hpp"

void transmitLoop(
    AudioFifo& micFifo,
    AudioFifo& playbackFifo,
    const std::function<bool()>& isActive,
    const std::function<void(bool)>& setLed,
    UdpSender* sender,
    bool simulationMode
) {
    struct LocalStats {
        WCETStats pop;
        WCETStats proc;
        uint64_t count{0};
    } stats;

    while (true) {
        auto t0 = std::chrono::steady_clock::now();

        AudioBlock packetBlock;
        packetBlock.samples.reserve(gFramesPerBuffer);

        bool gotAny = false;

        for (int i = 0; i < gBlocksPerPacket; ++i) {
            AudioBlock smallBlock = micFifo.pop();

            if (!gotAny) {
                packetBlock.captureNs = smallBlock.captureNs;
                packetBlock.sendNs = smallBlock.sendNs;
                gotAny = true;
            }

            packetBlock.samples.insert(
                packetBlock.samples.end(),
                smallBlock.samples.begin(),
                smallBlock.samples.end()
            );
        }

        auto t1 = std::chrono::steady_clock::now();
        uint64_t popLatency =
            std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();

        stats.pop.update(popLatency);
        gTimingLogger.add("transmit_pop_10x", stats.count + 1, popLatency);

        auto t2 = std::chrono::steady_clock::now();

        bool active = isActive();
        setLed(active);

        if (active) {
            if (sender != nullptr) {
                auto tSend0 = std::chrono::steady_clock::now();

                sender->sendBlock(packetBlock);

                auto tSend1 = std::chrono::steady_clock::now();
                uint64_t sendLatency =
                    std::chrono::duration_cast<std::chrono::nanoseconds>(tSend1 - tSend0).count();

                gTimingLogger.add("transmit_send", stats.count + 1, sendLatency);
            } else {
                auto tPush0 = std::chrono::steady_clock::now();

                playbackFifo.push(packetBlock);

                auto tPush1 = std::chrono::steady_clock::now();
                uint64_t pushLatency =
                    std::chrono::duration_cast<std::chrono::nanoseconds>(tPush1 - tPush0).count();

                gTimingLogger.add("transmit_local_push", stats.count + 1, pushLatency);
            }
        }

        auto t3 = std::chrono::steady_clock::now();
        uint64_t procLatency =
            std::chrono::duration_cast<std::chrono::nanoseconds>(t3 - t2).count();

        stats.proc.update(procLatency);
        gTimingLogger.add("transmit_proc", stats.count + 1, procLatency);

        ++stats.count;
    }
}

void receiveLoop(AudioFifo& playbackFifo, UdpReceiver& receiver) {
    uint64_t receivedCount = 0;
    WCETStats recvStats;
    WCETStats pushStats;

    const int PROCESS_FRAMES = gProcessFrames;

    while (true) {
        auto t0 = std::chrono::steady_clock::now();

        AudioBlock remoteBlock = receiver.receiveBlock(gFramesPerBuffer);

        auto t1 = std::chrono::steady_clock::now();
        uint64_t recvLatency =
            std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();

        if (remoteBlock.samples.empty()) {
            continue;
        }

        recvStats.update(recvLatency);
        gTimingLogger.add("receive_recv", receivedCount + 1, recvLatency);

        auto t2 = std::chrono::steady_clock::now();

        for (size_t offset = 0; offset < remoteBlock.samples.size(); offset += PROCESS_FRAMES) {
            size_t n = std::min<size_t>(PROCESS_FRAMES, remoteBlock.samples.size() - offset);

            AudioBlock smallBlock;
            smallBlock.captureNs = remoteBlock.captureNs;
            smallBlock.sendNs = remoteBlock.sendNs;

            smallBlock.samples.assign(
                remoteBlock.samples.begin() + offset,
                remoteBlock.samples.begin() + offset + n
            );

            playbackFifo.push(std::move(smallBlock));
        }

        auto t3 = std::chrono::steady_clock::now();
        uint64_t pushLatency =
            std::chrono::duration_cast<std::chrono::nanoseconds>(t3 - t2).count();

        pushStats.update(pushLatency);
        gTimingLogger.add("receive_push_10x", receivedCount + 1, pushLatency);

        ++receivedCount;
    }
}