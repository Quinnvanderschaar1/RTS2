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
        WCETStats combine;
        WCETStats send;
        uint64_t count{0};
    } stats;

    while (true) {
        std::vector<AudioBlock> blocks;
        blocks.reserve(gBlocksPerPacket);

        for (int i = 0; i < gBlocksPerPacket; ++i) {
            auto t0 = std::chrono::steady_clock::now();
            AudioBlock smallBlock;
            bool popped = micFifo.tryPop(smallBlock, false);
            auto t1 = std::chrono::steady_clock::now();
            uint64_t popLatency =
                std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
            stats.pop.update(popLatency);
            gTimingLogger.add("transmit_block_pop", stats.count + 1, popLatency);

            if (!popped) {
                blocks.clear();
                break;
            }

            blocks.push_back(std::move(smallBlock));
        }

        if (blocks.size() != static_cast<size_t>(gBlocksPerPacket)) {
            continue;
        }

        AudioBlock packetBlock;
        packetBlock.samples.reserve(gFramesPerBuffer);

        auto tCombine0 = std::chrono::steady_clock::now();
        bool gotAny = false;
        for (const auto& smallBlock : blocks) {
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
        auto tCombine1 = std::chrono::steady_clock::now();
        uint64_t combineLatency =
            std::chrono::duration_cast<std::chrono::nanoseconds>(tCombine1 - tCombine0).count();
        stats.combine.update(combineLatency);
        gTimingLogger.add("transmit_block_proc", stats.count + 1, combineLatency);

        bool active = isActive();
        setLed(active);

        if (active) {
            auto tSend0 = std::chrono::steady_clock::now();

            if (sender != nullptr) {
                sender->sendBlock(packetBlock);
            } else {
                playbackFifo.tryPush(packetBlock, false);
            }

            auto tSend1 = std::chrono::steady_clock::now();
            uint64_t sendLatency =
                std::chrono::duration_cast<std::chrono::nanoseconds>(tSend1 - tSend0).count();
            stats.send.update(sendLatency);
            gTimingLogger.add("transmit_send", stats.count + 1, sendLatency);
        }

        ++stats.count;
    }
}

void receiveLoop(AudioFifo& playbackFifo, UdpReceiver& receiver) {
    uint64_t receivedCount = 0;
    WCETStats splitStats;
    WCETStats pushStats;

    const int PROCESS_FRAMES = gProcessFrames;

    while (true) {
        AudioBlock remoteBlock = receiver.receiveBlock(gFramesPerBuffer);

        if (remoteBlock.samples.empty()) {
            continue;
        }

        auto tSplit0 = std::chrono::steady_clock::now();
        for (size_t offset = 0; offset < remoteBlock.samples.size(); offset += PROCESS_FRAMES) {
            size_t n = std::min<size_t>(PROCESS_FRAMES, remoteBlock.samples.size() - offset);
            AudioBlock smallBlock;
            smallBlock.captureNs = remoteBlock.captureNs;
            smallBlock.sendNs = remoteBlock.sendNs;

            smallBlock.samples.assign(
                remoteBlock.samples.begin() + offset,
                remoteBlock.samples.begin() + offset + n
            );

            auto tPush0 = std::chrono::steady_clock::now();
            bool pushed = playbackFifo.tryPush(std::move(smallBlock), false);
            auto tPush1 = std::chrono::steady_clock::now();
            uint64_t pushLatency =
                std::chrono::duration_cast<std::chrono::nanoseconds>(tPush1 - tPush0).count();
            if (pushed) {
                pushStats.update(pushLatency);
                gTimingLogger.add("receive_Block_push", receivedCount + 1, pushLatency);
            }
        }
        auto tSplit1 = std::chrono::steady_clock::now();
        uint64_t splitLatency =
            std::chrono::duration_cast<std::chrono::nanoseconds>(tSplit1 - tSplit0).count();
        splitStats.update(splitLatency);
        gTimingLogger.add("receive_block_proc", receivedCount + 1, splitLatency);

        ++receivedCount;
    }
}