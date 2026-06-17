#include "ProcessingThreads.hpp"
#include "TimingLogger.hpp"

#include <chrono>

void lowPassThreadLoop(
    AudioFifo& source,
    AudioFifo& destination,
    AudioProcessing& processing,
    float alpha
) {
    uint64_t lowpassCount = 0;

    while (true) {
        auto tPop0 = std::chrono::steady_clock::now();
        AudioBlock block = source.pop();
        auto tPop1 = std::chrono::steady_clock::now();
        uint64_t popLatency = std::chrono::duration_cast<std::chrono::nanoseconds>(tPop1 - tPop0).count();
        gTimingLogger.add("lowpass_pop", lowpassCount + 1, popLatency);

        auto tProc0 = std::chrono::steady_clock::now();
        block.samples = processing.lowPass(block.samples, alpha);
        auto tProc1 = std::chrono::steady_clock::now();
        uint64_t procLatency = std::chrono::duration_cast<std::chrono::nanoseconds>(tProc1 - tProc0).count();
        gTimingLogger.add("lowpass_proc", lowpassCount + 1, procLatency);

        auto tPush0 = std::chrono::steady_clock::now();
        destination.push(block);
        auto tPush1 = std::chrono::steady_clock::now();
        uint64_t pushLatency = std::chrono::duration_cast<std::chrono::nanoseconds>(tPush1 - tPush0).count();
        gTimingLogger.add("lowpass_push", lowpassCount + 1, pushLatency);

        ++lowpassCount;
    }
}

void echoCancelThreadLoop(
    AudioFifo& source,
    AudioFifo& destination,
    AudioProcessing& processing,
    int delaySamples,
    float decay
) {
    uint64_t echoCount = 0;

    while (true) {
        auto tPop0 = std::chrono::steady_clock::now();
        AudioBlock block = source.pop();
        auto tPop1 = std::chrono::steady_clock::now();
        uint64_t popLatency = std::chrono::duration_cast<std::chrono::nanoseconds>(tPop1 - tPop0).count();
        gTimingLogger.add("echocancel_pop", echoCount + 1, popLatency);

        auto tProc0 = std::chrono::steady_clock::now();
        block.samples = processing.echoCancellation(block.samples, delaySamples, decay);
        auto tProc1 = std::chrono::steady_clock::now();
        uint64_t procLatency = std::chrono::duration_cast<std::chrono::nanoseconds>(tProc1 - tProc0).count();
        gTimingLogger.add("echocancel_proc", echoCount + 1, procLatency);

        auto tPush0 = std::chrono::steady_clock::now();
        destination.push(block);
        auto tPush1 = std::chrono::steady_clock::now();
        uint64_t pushLatency = std::chrono::duration_cast<std::chrono::nanoseconds>(tPush1 - tPush0).count();
        gTimingLogger.add("echocancel_push", echoCount + 1, pushLatency);

        ++echoCount;
    }
}
