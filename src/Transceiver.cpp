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
        auto fullStart = std::chrono::steady_clock::now();

        AudioBlock packetBlock;
        packetBlock.samples.reserve(gFramesPerBuffer);

        bool gotAny = false;
        uint64_t totalPopLatency = 0;
        uint64_t totalBlockAssemblyLatency = 0;

        for (int i = 0; i < gBlocksPerPacket; ++i) {
            auto popStart = std::chrono::steady_clock::now();

            AudioBlock smallBlock = micFifo.pop();

            auto popEnd = std::chrono::steady_clock::now();

            uint64_t popLatency =
                std::chrono::duration_cast<std::chrono::nanoseconds>(
                    popEnd - popStart
                ).count();

            totalPopLatency += popLatency;
            stats.pop.update(popLatency);

            gTimingLogger.add("transmit_block_pop", stats.count + 1, popLatency);

            if (popLatency > 30000000) {
                std::cout
                    << "[SEND] slow micFifo.pop i="
                    << i
                    << " popMs="
                    << popLatency / 1000000.0
                    << std::endl;
            }
            auto BlockAssemblyStart = std::chrono::steady_clock::now();
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
            auto BlockAssemblyEnd = std::chrono::steady_clock::now();
            uint64_t blockAssemblyLatency =
                std::chrono::duration_cast<std::chrono::nanoseconds>(
                    BlockAssemblyEnd - BlockAssemblyStart
                ).count();
            totalBlockAssemblyLatency += blockAssemblyLatency;
        }

        gTimingLogger.add("transmit_pop_total", stats.count + 1, totalPopLatency);
        gTimingLogger.add("transmit_block_assembly_total", stats.count + 1, totalBlockAssemblyLatency);

        auto activeStart = std::chrono::steady_clock::now();

        bool active = isActive();
        setLed(active);

        if (active) {
            if (sender != nullptr) {
                auto sendStart = std::chrono::steady_clock::now();

                sender->sendBlock(packetBlock);

                auto sendEnd = std::chrono::steady_clock::now();

                uint64_t sendLatency =
                    std::chrono::duration_cast<std::chrono::nanoseconds>(
                        sendEnd - sendStart
                    ).count();

                gTimingLogger.add("transmit_send", stats.count + 1, sendLatency);
            } else {
                auto pushStart = std::chrono::steady_clock::now();

                playbackFifo.push(packetBlock);

                auto pushEnd = std::chrono::steady_clock::now();

                uint64_t pushLatency =
                    std::chrono::duration_cast<std::chrono::nanoseconds>(
                        pushEnd - pushStart
                    ).count();

                gTimingLogger.add("transmit_push", stats.count + 1, pushLatency);
            }
        }

        auto fullEnd = std::chrono::steady_clock::now();

        uint64_t fullLatency =
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                fullEnd - fullStart
            ).count();

        uint64_t activeLatency =
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                fullEnd - activeStart
            ).count();

        stats.proc.update(fullLatency);

        gTimingLogger.add("transmit_active_send_or_push", stats.count + 1, activeLatency);
        gTimingLogger.add("transmit_full_delay", stats.count + 1, fullLatency);

        ++stats.count;
    }
}

void receiveLoop(AudioFifo& playbackFifo, UdpReceiver& receiver) {
    uint64_t receivedCount = 0;
    WCETStats recvStats;
    WCETStats pushStats;

    const int PROCESS_FRAMES = gProcessFrames;

    while (true) {
        uint64_t totalBlockDisassemblyLatency = 0;
        auto recvStart = std::chrono::steady_clock::now();

        AudioBlock remoteBlock = receiver.receiveBlock(gFramesPerBuffer);

        auto recvEnd = std::chrono::steady_clock::now();

        uint64_t recvLatency =
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                recvEnd - recvStart
            ).count();

        if (remoteBlock.samples.empty()) {
            continue;
        }

        recvStats.update(recvLatency);
        gTimingLogger.add("receive_recv_delay", receivedCount + 1, recvLatency);

        auto splitPushStart = std::chrono::steady_clock::now();

        size_t pushedBlocks = 0;

        for (size_t offset = 0; offset < remoteBlock.samples.size(); offset += PROCESS_FRAMES) {
            auto splitStart = std::chrono::steady_clock::now();
            size_t n = std::min<size_t>(
                PROCESS_FRAMES,
                remoteBlock.samples.size() - offset
            );

            AudioBlock smallBlock;
            smallBlock.captureNs = remoteBlock.captureNs;
            smallBlock.sendNs = remoteBlock.sendNs;

            smallBlock.samples.assign(
                remoteBlock.samples.begin() + offset,
                remoteBlock.samples.begin() + offset + n
            );
            auto splitEnd = std::chrono::steady_clock::now();

            uint64_t splitLatency =
                std::chrono::duration_cast<std::chrono::nanoseconds>(
                    splitEnd - splitStart
                ).count();

            totalBlockDisassemblyLatency += splitLatency;

            auto pushStart = std::chrono::steady_clock::now();

            playbackFifo.push(std::move(smallBlock));

            auto pushEnd = std::chrono::steady_clock::now();

            uint64_t pushLatency =
                std::chrono::duration_cast<std::chrono::nanoseconds>(
                    pushEnd - pushStart
                ).count();

            pushStats.update(pushLatency);
            gTimingLogger.add("receive_block_push", receivedCount + 1, pushLatency);

            ++pushedBlocks;
        }

        auto splitPushEnd = std::chrono::steady_clock::now();

        uint64_t splitPushLatency =
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                splitPushEnd - splitPushStart
            ).count();

        gTimingLogger.add("receive_block_disassembly_total", receivedCount + 1, totalBlockDisassemblyLatency);
        gTimingLogger.add("receive_split_and_push_total", receivedCount + 1, splitPushLatency);
        ++receivedCount;
    }
}
