#include "AudioFifo.hpp"
#include "AudioRecorder.hpp"
#include "AudioPlayer.hpp"
#include "UserInterface.hpp"
#include "UdpSender.hpp"
#include "UdpReceiver.hpp"

#include <pthread.h>
#include <portaudio.h>
#include <vector>
#include <iostream>
#include <cstdint>

constexpr int SAMPLE_RATE = 48000;
constexpr int CHANNELS = 1;
constexpr int FRAMES_10MS = SAMPLE_RATE / 100;

constexpr const char* UDP_GROUP = "192.168.50.189";
constexpr uint16_t UDP_PORT = 5005;

constexpr int BUTTON_GPIO = 17;
constexpr int LED_GPIO = 27;

struct TransceiverArgs {
    AudioFifo* micFifo;
    AudioFifo* playbackFifo;
    UserInterface* ui;
    UdpSender* sender;
    UdpReceiver* receiver;
};

void* recorder_thread(void* arg) {
    static_cast<AudioRecorder*>(arg)->start();
    return nullptr;
}

void* player_thread(void* arg) {
    static_cast<AudioPlayer*>(arg)->start();
    return nullptr;
}

void* transmit_thread(void* arg) {
    auto* args = static_cast<TransceiverArgs*>(arg);

    while (true) {
        std::vector<float> micBlock = args->micFifo->pop();

        bool active = args->ui->isButtonPressed();
        args->ui->setLed(active);

        if (active) {
            args->sender->sendData(micBlock);
        }
    }

    return nullptr;
}

void* receive_thread(void* arg) {
    auto* args = static_cast<TransceiverArgs*>(arg);

    int receivedCount = 0;

    while (true) {
        std::vector<float> remoteBlock =
            args->receiver->receiveFloatData(FRAMES_10MS * CHANNELS);

        if (!remoteBlock.empty()) {
            args->playbackFifo->push(remoteBlock);

            receivedCount++;

            if (receivedCount % 100 == 0) {
                std::cout << "Received audio packets: "
                          << receivedCount
                          << " | samples: "
                          << remoteBlock.size()
                          << std::endl;
            }
        }
    }

    return nullptr;
}

int main(int argc, char* argv[]) {
    const char* udpGroup = UDP_GROUP;

    if (argc >= 2) {
        udpGroup = argv[1];
    }

    std::cout << "Using UDP address: " << udpGroup << std::endl;

    Pa_Initialize();

    AudioFifo micFifo;
    AudioFifo playbackFifo;

    AudioRecorder recorder(micFifo);
    AudioPlayer player(playbackFifo);

    UserInterface ui(BUTTON_GPIO, LED_GPIO);

    UdpSender sender(udpGroup, UDP_PORT);
    UdpReceiver receiver(udpGroup, UDP_PORT);

    TransceiverArgs transceiverArgs{
        &micFifo,
        &playbackFifo,
        &ui,
        &sender,
        &receiver
    };

    pthread_t recorderThread;
    pthread_t playerThread;
    pthread_t transmitThread;
    pthread_t receiveThreadId;

    pthread_create(&recorderThread, nullptr, recorder_thread, &recorder);
    pthread_create(&playerThread, nullptr, player_thread, &player);
    pthread_create(&transmitThread, nullptr, transmit_thread, &transceiverArgs);
    pthread_create(&receiveThreadId, nullptr, receive_thread, &transceiverArgs);

    std::cout << "Conferencing started..." << std::endl;
    std::cout << "Press SPACE to toggle transmit ON/OFF." << std::endl;

    pthread_join(recorderThread, nullptr);
    pthread_join(playerThread, nullptr);
    pthread_join(transmitThread, nullptr);
    pthread_join(receiveThreadId, nullptr);

    Pa_Terminate();

    return 0;
}