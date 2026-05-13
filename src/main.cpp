#include "AudioFifo.hpp"
#include "AudioRecorder.hpp"
#include "AudioPlayer.hpp"
#include "UdpSender.hpp"
#include "UdpReceiver.hpp"
#include "AudioMixer.hpp"

#include <pthread.h>
#include <portaudio.h>
#include <vector>
#include <iostream>

constexpr int SAMPLE_RATE = 48000;
constexpr int CHANNELS = 1;
constexpr int FRAMES_10MS = SAMPLE_RATE / 100;

constexpr const char* UDP_GROUP = "192.168.50.138";
constexpr uint16_t UDP_PORT = 5005;

struct NetworkSenderArgs {
    AudioFifo* micFifo;
    UdpSender* sender;
};

struct NetworkReceiverArgs {
    AudioFifo* remoteFifo;
    UdpReceiver* receiver;
};

struct MixerArgs {
    AudioFifo* micFifo;
    AudioFifo* remoteFifo;
    AudioFifo* playbackFifo;
    AudioMixer* mixer;
};

void* recorder_thread(void* arg) {
    auto* recorder = static_cast<AudioRecorder*>(arg);
    recorder->start();
    return nullptr;
}

void* player_thread(void* arg) {
    auto* player = static_cast<AudioPlayer*>(arg);
    player->start();
    return nullptr;
}

void* network_sender_thread(void* arg) {
    auto* args = static_cast<NetworkSenderArgs*>(arg);

    while (true) {
        std::vector<float> micBlock = args->micFifo->pop();
        args->sender->sendData(micBlock);

        // Put mic data back so the mixer can also use it
        args->micFifo->push(micBlock);
    }

    return nullptr;
}

void* network_receiver_thread(void* arg) {
    auto* args = static_cast<NetworkReceiverArgs*>(arg);

    while (true) {
        std::vector<float> remoteBlock =
            args->receiver->receiveFloatData(FRAMES_10MS * CHANNELS);

        if (!remoteBlock.empty()) {
            args->remoteFifo->push(remoteBlock);
        }
    }

    return nullptr;
}

void* mixer_thread(void* arg) {
    auto* args = static_cast<MixerArgs*>(arg);

    while (true) {
        std::vector<float> micBlock = args->micFifo->pop();
        std::vector<float> remoteBlock = args->remoteFifo->pop();

        std::vector<float> mixed = args->mixer->mix({
            micBlock,
            remoteBlock
        });

        args->playbackFifo->push(mixed);
    }

    return nullptr;
}

int main() {
    Pa_Initialize();

    AudioFifo micFifo;
    AudioFifo remoteFifo;
    AudioFifo playbackFifo;

    AudioRecorder recorder(micFifo);
    AudioPlayer player(playbackFifo);

    UdpSender sender(UDP_GROUP, UDP_PORT);
    UdpReceiver receiver(UDP_GROUP, UDP_PORT);

    AudioMixer mixer;

    NetworkSenderArgs senderArgs{
        &micFifo,
        &sender
    };

    NetworkReceiverArgs receiverArgs{
        &remoteFifo,
        &receiver
    };

    MixerArgs mixerArgs{
        &micFifo,
        &remoteFifo,
        &playbackFifo,
        &mixer
    };

    pthread_t recorderThread;
    pthread_t playerThread;
    pthread_t senderThread;
    pthread_t receiverThread;
    pthread_t mixerThreadId;

    pthread_create(&recorderThread, nullptr, recorder_thread, &recorder);
    pthread_create(&playerThread, nullptr, player_thread, &player);
    pthread_create(&senderThread, nullptr, network_sender_thread, &senderArgs);
    pthread_create(&receiverThread, nullptr, network_receiver_thread, &receiverArgs);
    pthread_create(&mixerThreadId, nullptr, mixer_thread, &mixerArgs);

    std::cout << "Conferencing started..." << std::endl;

    pthread_join(recorderThread, nullptr);
    pthread_join(playerThread, nullptr);
    pthread_join(senderThread, nullptr);
    pthread_join(receiverThread, nullptr);
    pthread_join(mixerThreadId, nullptr);

    Pa_Terminate();

    return 0;
}