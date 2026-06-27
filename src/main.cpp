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
#include "ProcessingThreads.hpp"
#include "SimulatedAudio.hpp"
#include "SimulatedUI.hpp"
#include "Transceiver.hpp"
#include "wcet.hpp"
#include "TimingLogger.hpp"
#include "Globals.hpp"

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
#include <atomic>
#include <pthread.h>

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
constexpr int DEFAULT_ECHO_DELAY_SAMPLES = FRAMES_10MS;
constexpr float ECHO_DECAY = 0.35f;

static void setThreadName(const char* name) {
    pthread_setname_np(pthread_self(), name); // Linux limit: 15 chars + null
}

int main(int argc, char* argv[]) {
    bool simulationMode = false;
    std::string udpGroup = UDP_GROUP;
    bool useUdp = true;
    bool userSpecifiedUdp = false;
    bool useProcessing = true;

    int audioMs = 10;
    int splitDivisor = 10;
    int echoDelaySamples = DEFAULT_ECHO_DELAY_SAMPLES;

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
        } else if (arg == "--ms" && i + 1 < argc) {
            audioMs = std::atoi(argv[++i]);
        } else if (arg == "--split" && i + 1 < argc) {
            splitDivisor = std::atoi(argv[++i]);
        } else if (arg == "--fifo" && i + 1 < argc) {
            FIFO_SIZE = std::atoi(argv[++i]);
        } else if (!hasPrefix(arg, "--")) {
            udpGroup = arg;
            userSpecifiedUdp = true;
            simulationMode = false;
        }
    }

    if (audioMs <= 0) {
        std::cerr << "Invalid --ms value. Must be greater than 0." << std::endl;
        return 1;
    }

    if (splitDivisor <= 0) {
        std::cerr << "Invalid --split value. Must be greater than 0." << std::endl;
        return 1;
    }

    gAudioMs = audioMs;
    gSplitDivisor = splitDivisor;

    gFramesPerBuffer = SAMPLE_RATE * gAudioMs / 1000;

    if (gFramesPerBuffer <= 0) {
        std::cerr << "Invalid --ms value. Frames per buffer became 0." << std::endl;
        return 1;
    }

    if (gFramesPerBuffer % gSplitDivisor != 0) {
        std::cerr << "Invalid --split value. Frames per buffer must be divisible by split divisor." << std::endl;
        return 1;
    }

    gProcessFrames = gFramesPerBuffer / gSplitDivisor;
    gBlocksPerPacket = gSplitDivisor;

    echoDelaySamples = gFramesPerBuffer;
    if (echoDelaySamples <= 0) {
        std::cerr << "Invalid --div value. SAMPLE_RATE / div must be greater than 0." << std::endl;
        return 1;
    }

    std::atexit([]() {
        gTimingLogger.saveCSV("timing_report.csv");
        gTimingLogger.saveFifoCSV("fifo_occupancy.csv");
        gTimingLogger.saveQueueCSV("queue_latency.csv");
        gTimingLogger.saveNetworkCSV("network_jitter.csv");
    });

    std::cout << "Mode: " << (simulationMode ? "simulation" : "hardware") << std::endl;
    std::cout << "Audio processing: " << (useProcessing ? "enabled" : "disabled") << std::endl;
    std::cout << "Audio block ms: " << gAudioMs << std::endl;
    std::cout << "Frames per buffer: " << gFramesPerBuffer << std::endl;
    std::cout << "Split divisor: " << gSplitDivisor << std::endl;
    std::cout << "Process frames: " << gProcessFrames << std::endl;
    std::cout << "Blocks per packet: " << gBlocksPerPacket << std::endl;

    std::cout << "Echo delay samples: " << echoDelaySamples << std::endl;

    if (!simulationMode) {
        std::cout << "Using UDP address: " << udpGroup << std::endl;
    } else {
        std::cout << "Simulation active: microphone and playback are both simulated." << std::endl;
    }

    std::cout << "Press SPACE to toggle transmit ON/OFF." << std::endl;
    std::cout << "Press X to save timing_report.csv and exit." << std::endl;

    AudioFifo micFifo(FIFO_SIZE);
    AudioFifo lowPassFifo(FIFO_SIZE);
    AudioFifo echoCancelFifo(FIFO_SIZE);
    AudioFifo audioEncoderFifo(FIFO_SIZE);
    AudioFifo audioDecoderFifo(FIFO_SIZE);
    AudioFifo playbackFifo(FIFO_SIZE);

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

    std::atomic<bool> transmitActive{false};
    std::thread uiThread;

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
        PaError paErr = Pa_Initialize();

        if (paErr != paNoError) {
            std::cerr << "Pa_Initialize failed: "
                      << Pa_GetErrorText(paErr) << std::endl;
            return 1;
        }

        recorder = std::make_unique<AudioRecorder>(micFifo, &endToEndStats);
        player = std::make_unique<AudioPlayer>(playbackFifo, &endToEndStats);
        ui = std::make_unique<UserInterface>(BUTTON_GPIO, LED_GPIO);
        sender = std::make_unique<UdpSender>(udpGroup, UDP_PORT);
        receiver = std::make_unique<UdpReceiver>(udpGroup, UDP_PORT);

        uiThread = std::thread([&] {
            setThreadName("ui");
            while (true) {
                bool active = ui->isButtonPressed();
                transmitActive.store(active);
                ui->setLed(active);
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        });

        isActive = [&]() {
            return transmitActive.load();
        };

        setLed = [](bool) {};
#else
        std::cerr << "Hardware mode is disabled in this build. Run without -DUSE_SIMULATION=ON." << std::endl;
        return 1;
#endif
    }

    std::thread recorderThread([&] {
        setThreadName("recorder");
        std::cerr << "[THREAD] recorder thread running" << std::endl;
        if (simulationMode) {
            recorderSim->start();
        } else {
#ifndef USE_SIMULATION
            recorder->start();
#endif
        }
    });

    std::thread playerThread([&] {
        setThreadName("player");
        std::cerr << "[THREAD] player thread running" << std::endl;
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
    std::thread audioEncoderThread;
    std::thread audioDecoderThread;
    std::thread transmitThread;

    if (useProcessing) {
        lowPassThread = std::thread([&] {
            setThreadName("lowpass");
            lowPassThreadLoop(
            std::ref(micFifo),
            std::ref(lowPassFifo),
            std::ref(audioProcessing),
            LOW_PASS_ALPHA
            );
        });

        echoCancelThread = std::thread([&] {
            setThreadName("echo_cancel");
            echoCancelThreadLoop(
            std::ref(lowPassFifo),
            std::ref(echoCancelFifo),
            std::ref(audioProcessing),
            echoDelaySamples,
            ECHO_DECAY
            );
        });

        audioEncoderThread = std::thread([&] {
            setThreadName("Encoding");
            audioEncoderThreadLoop(
            std::ref(echoCancelFifo),
            std::ref(audioEncoderFifo),
            std::ref(audioProcessing)
            );
        });

        audioDecoderThread = std::thread([&] {
            setThreadName("Decoding");
            audioDecoderThreadLoop(
            std::ref(audioDecoderFifo),
            std::ref(playbackFifo),
            std::ref(audioProcessing)
            );
        });

        transmitThread = std::thread([&] {
            setThreadName("transmit");
            transmitLoop(
                audioEncoderFifo,
                playbackFifo,
                isActive,
                setLed,
                useUdp ? sender.get() : nullptr,
                simulationMode
            );
        });
    } else {
        transmitThread = std::thread([&] {
            setThreadName("transmit");
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

    if (useUdp && receiver && useProcessing) {
        receiveThread = std::thread([&] {
            setThreadName("receive");
            receiveLoop(audioDecoderFifo, *receiver);
        });
    }
    else if (useUdp && receiver) {
        receiveThread = std::thread([&] {
            setThreadName("receive");
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

    if (echoCancelThread.joinable()) {
        echoCancelThread.join();
    }

    if (audioEncoderThread.joinable()) {
        audioEncoderThread.join();
    }

    if (audioDecoderThread.joinable()) {
        audioDecoderThread.join();
    }

    transmitThread.join();

    if (receiveThread.joinable()) {
        receiveThread.join();
    }

    if (uiThread.joinable()) {
        uiThread.join();
    }

    if (!simulationMode) {
#ifndef USE_SIMULATION
        Pa_Terminate();
#endif
    }

    return 0;
}