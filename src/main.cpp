#include "AudioFifo.hpp"
#ifndef USE_SIMULATION
#include "AudioRecorder.hpp"
#include "AudioPlayer.hpp"
#endif
#include "UserInterface.hpp"
#include "UdpSender.hpp"
#include "UdpReceiver.hpp"
#include "AudioMixer.hpp"
#include "AudioProcessing.hpp"
#include "SimulatedAudio.hpp"
#include "SimulatedUI.hpp"
#include "Transceiver.hpp"
#include "wcet.hpp"
#include "TimingLogger.hpp"

#include <chrono>
#include <cstring>
#include <functional>
#include <iostream>
#include <thread>
#include <vector>
#include <cstdint>
#include <cstdlib>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

#ifndef USE_SIMULATION
#include <portaudio.h>
#endif

constexpr int SAMPLE_RATE = 48000;
constexpr int CHANNELS = 1;
constexpr int FRAMES_10MS = SAMPLE_RATE / 100;
constexpr const char* UDP_GROUP = "192.168.50.189";
constexpr uint16_t UDP_PORT = 5005;
constexpr int BUTTON_GPIO = 17;
constexpr int LED_GPIO = 27;

constexpr float LOW_PASS_ALPHA = 0.15f;
constexpr int ECHO_DELAY_SAMPLES = FRAMES_10MS;
constexpr float ECHO_DECAY = 0.35f;

int main(int argc, char* argv[]) {
    bool simulationMode = true;
    std::string udpGroup = UDP_GROUP;
    bool useUdp = true;
    bool userSpecifiedUdp = false;
    bool useProcessing = true;

    auto hasPrefix = [](const std::string& value, const std::string& prefix) {
        return value.size() >= prefix.size() &&
               value.compare(0, prefix.size(), prefix) == 0;
    };

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--hw") {
            simulationMode = false;
        } else if (arg == "--sim") {
            simulationMode = true;
        } else if (arg == "--no-udp") {
            useUdp = false;
        } else if (arg == "--np") {
            useProcessing = false;
        } else if (!hasPrefix(arg, "--")) {
            udpGroup = arg;
            userSpecifiedUdp = true;
            simulationMode = false;
        }
    }

    std::atexit([]() {
        gTimingLogger.saveCSV("timing_report.csv");
        gTimingLogger.saveFifoCSV("fifo_occupancy.csv");
        gTimingLogger.saveQueueCSV("queue_latency.csv");
        gTimingLogger.saveNetworkCSV("network_jitter.csv");
    });

    std::cout << "Mode: " << (simulationMode ? "simulation" : "hardware") << std::endl;
    std::cout << "Audio processing: "
              << (useProcessing ? "enabled" : "disabled")
              << std::endl;

    if (!simulationMode) {
        std::cout << "Using UDP address: " << udpGroup << std::endl;
    } else {
        std::cout << "Simulation active: microphone and playback are both simulated." << std::endl;
        std::cout << "Press SPACE to toggle transmit ON/OFF." << std::endl;
        std::cout << "Press X to save timing_report.csv and exit." << std::endl;
    }

    AudioFifo micFifo(8);
    AudioFifo lowPassFifo(8);
    AudioFifo echoCancelFifo(8);
    AudioFifo playbackFifo(8);

    AudioProcessing audioProcessing;

#ifndef USE_SIMULATION
    std::unique_ptr<AudioRecorder> recorder;
    std::unique_ptr<AudioPlayer> player;
#endif

    std::unique_ptr<AudioRecorderSimulator> recorderSim;
    std::unique_ptr<AudioPlayerSimulator> playerSim;
    std::unique_ptr<UserInterface> ui;
    std::unique_ptr<SimulatedUserInterface> simulatedUi;
    std::unique_ptr<UdpSender> sender;
    std::unique_ptr<UdpReceiver> receiver;

    std::function<bool()> isActive;
    std::function<void(bool)> setLed;

    WCETStats endToEndStats;

    if (simulationMode) {
        if (!userSpecifiedUdp) {
            udpGroup = "127.0.0.1";
        }

        simulatedUi = std::make_unique<SimulatedUserInterface>();

        recorderSim = std::make_unique<AudioRecorderSimulator>(
            micFifo,
            useUdp ? &endToEndStats : nullptr
        );

        playerSim = std::make_unique<AudioPlayerSimulator>(
            playbackFifo,
            useUdp ? &endToEndStats : nullptr
        );

        if (useUdp) {
            sender = std::make_unique<UdpSender>(udpGroup, UDP_PORT);
            receiver = std::make_unique<UdpReceiver>(udpGroup, UDP_PORT);
        }

        isActive = [thisSimulation = simulatedUi.get()]() {
            return thisSimulation->isButtonPressed();
        };

        setLed = [](bool) {};
    } else {
#ifndef USE_SIMULATION
        Pa_Initialize();

        recorder = std::make_unique<AudioRecorder>(micFifo, &endToEndStats);
        player = std::make_unique<AudioPlayer>(playbackFifo, &endToEndStats);
        ui = std::make_unique<UserInterface>(BUTTON_GPIO, LED_GPIO);
        sender = std::make_unique<UdpSender>(udpGroup, UDP_PORT);
        receiver = std::make_unique<UdpReceiver>(udpGroup, UDP_PORT);

        isActive = [thisUi = ui.get()]() {
            return thisUi->isButtonPressed();
        };

        setLed = [thisUi = ui.get()](bool on) {
            thisUi->setLed(on);
        };
#else
        std::cerr << "Hardware mode is disabled in this build. Run without -DUSE_SIMULATION=ON." << std::endl;
        return 1;
#endif
    }

    std::thread recorderThread([&] {
        if (simulationMode) {
            recorderSim->start();
        } else {
#ifndef USE_SIMULATION
            recorder->start();
#endif
        }
    });

    std::thread playerThread([&] {
        if (simulationMode) {
            playerSim->start();
        } else {
#ifndef USE_SIMULATION
            player->start();
#endif
        }
    });

    std::thread lowPassThread;
    std::thread echoCancelThread;
    std::thread transmitThread;

    if (useProcessing) {
        lowPassThread = std::thread([&] {
            while (true) {
                AudioBlock block = micFifo.pop();

                block.samples = audioProcessing.lowPass(
                    block.samples,
                    LOW_PASS_ALPHA
                );

                lowPassFifo.push(block);
            }
        });

        echoCancelThread = std::thread([&] {
            while (true) {
                AudioBlock block = lowPassFifo.pop();

                block.samples = audioProcessing.echoCancellation(
                    block.samples,
                    ECHO_DELAY_SAMPLES,
                    ECHO_DECAY
                );

                echoCancelFifo.push(block);
            }
        });

        transmitThread = std::thread([&] {
            transmitLoop(
                echoCancelFifo,
                playbackFifo,
                isActive,
                setLed,
                useUdp ? sender.get() : nullptr,
                simulationMode
            );
        });
    } else {
        transmitThread = std::thread([&] {
            transmitLoop(
                micFifo,
                playbackFifo,
                isActive,
                setLed,
                useUdp ? sender.get() : nullptr,
                simulationMode
            );
        });
    }

    std::thread receiveThread;

    if (useUdp && receiver) {
        receiveThread = std::thread([&] {
            receiveLoop(playbackFifo, *receiver);
        });
    }

    std::cout << "Conferencing started..." << std::endl;

    recorderThread.join();
    playerThread.join();

    if (lowPassThread.joinable()) {
        lowPassThread.join();
    }

    if (echoCancelThread.joinable()) {
        echoCancelThread.join();
    }

    transmitThread.join();

    if (receiveThread.joinable()) {
        receiveThread.join();
    }

    if (!simulationMode) {
#ifndef USE_SIMULATION
        Pa_Terminate();
#endif
    }

    return 0;
}