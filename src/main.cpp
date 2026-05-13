#include "AudioFifo.hpp"
#include "AudioRecorder.hpp"
#include "AudioPlayer.hpp"
#include "UdpSender.hpp"
#include "UdpReceiver.hpp"
#include "AudioMixer.hpp"
#include "AudioProcessing.hpp"
#include "UserInterface.hpp"

#include <pthread.h>
#include <portaudio.h>
#include <vector>
#include <iostream>
#include <unistd.h>

constexpr int SAMPLE_RATE = 48000;
constexpr int CHANNELS = 1;
constexpr int FRAMES_10MS = SAMPLE_RATE / 100;

constexpr const char* UDP_GROUP = "192.168.50.138";
constexpr uint16_t UDP_PORT = 5005;

constexpr int BUTTON_GPIO = 17;
constexpr int LED_GPIO = 27;

struct NetworkSenderArgs {
    AudioFifo* micFifo;
    UdpSender* sender;
    UserInterface* ui;
};

struct NetworkReceiverArgs {
    AudioFifo* remoteFifo;
    UdpReceiver* receiver;
};

struct MixerArgs {
    AudioFifo* micFifo;
    AudioFifo* remoteFifo;
    AudioFifo* lowPassInputFifo;
    AudioFifo* echoInputFifo;
    AudioMixer* mixer;
};

struct ProcessingArgs {
    AudioFifo* inputFifo;
    AudioFifo* outputFifo;
    AudioProcessing* processing;
};

struct FinalMixArgs {
    AudioFifo* lowPassOutputFifo;
    AudioFifo* echoOutputFifo;
    AudioFifo* playbackFifo;
    AudioMixer* mixer;
};

void* recorder_thread(void* arg) {
    static_cast<AudioRecorder*>(arg)->start();
    return nullptr;
}

void* player_thread(void* arg) {
    static_cast<AudioPlayer*>(arg)->start();
    return nullptr;
}

void* network_sender_thread(void* arg) {
    auto* args = static_cast<NetworkSenderArgs*>(arg);

    while (true) {
        std::vector<float> micBlock = args->micFifo->pop();

        bool pressed = args->ui->isButtonPressed();
        args->ui->setLed(pressed);

        if (pressed) {
            args->sender->sendData(micBlock);
        }
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

        args->lowPassInputFifo->push(mixed);
        args->echoInputFifo->push(mixed);
    }

    return nullptr;
}

void* low_pass_thread(void* arg) {
    auto* args = static_cast<ProcessingArgs*>(arg);

    while (true) {
        std::vector<float> input = args->inputFifo->pop();

        std::vector<float> output =
            args->processing->lowPass(input, 0.1f);

        args->outputFifo->push(output);
    }

    return nullptr;
}

void* echo_cancellation_thread(void* arg) {
    auto* args = static_cast<ProcessingArgs*>(arg);

    while (true) {
        std::vector<float> input = args->inputFifo->pop();

        std::vector<float> output =
            args->processing->echoCancellation(
                input,
                2400,   // 50 ms at 48 kHz
                0.5f
            );

        args->outputFifo->push(output);
    }

    return nullptr;
}

void* final_mix_thread(void* arg) {
    auto* args = static_cast<FinalMixArgs*>(arg);

    while (true) {
        std::vector<float> lowPassBlock = args->lowPassOutputFifo->pop();
        std::vector<float> echoBlock = args->echoOutputFifo->pop();

        std::vector<float> finalBlock = args->mixer->mix({
            lowPassBlock,
            echoBlock
        });

        args->playbackFifo->push(finalBlock);
    }

    return nullptr;
}

int main() {
    Pa_Initialize();

    AudioFifo micFifo;
    AudioFifo remoteFifo;

    AudioFifo lowPassInputFifo;
    AudioFifo echoInputFifo;

    AudioFifo lowPassOutputFifo;
    AudioFifo echoOutputFifo;

    AudioFifo playbackFifo;

    AudioRecorder recorder(micFifo);
    AudioPlayer player(playbackFifo);

    UdpSender sender(UDP_GROUP, UDP_PORT);
    UdpReceiver receiver(UDP_GROUP, UDP_PORT);

    AudioMixer mixer;
    AudioProcessing processing;
    UserInterface ui(BUTTON_GPIO, LED_GPIO);

    NetworkSenderArgs senderArgs{
        &micFifo,
        &sender,
        &ui
    };

    NetworkReceiverArgs receiverArgs{
        &remoteFifo,
        &receiver
    };

    MixerArgs mixerArgs{
        &micFifo,
        &remoteFifo,
        &lowPassInputFifo,
        &echoInputFifo,
        &mixer
    };

    ProcessingArgs lowPassArgs{
        &lowPassInputFifo,
        &lowPassOutputFifo,
        &processing
    };

    ProcessingArgs echoArgs{
        &echoInputFifo,
        &echoOutputFifo,
        &processing
    };

    FinalMixArgs finalMixArgs{
        &lowPassOutputFifo,
        &echoOutputFifo,
        &playbackFifo,
        &mixer
    };

    pthread_t recorderThread;
    pthread_t playerThread;
    pthread_t senderThread;
    pthread_t receiverThread;
    pthread_t mixerThreadId;
    pthread_t lowPassThread;
    pthread_t echoThread;
    pthread_t finalMixThreadId;

    pthread_create(&recorderThread, nullptr, recorder_thread, &recorder);
    pthread_create(&playerThread, nullptr, player_thread, &player);
    pthread_create(&senderThread, nullptr, network_sender_thread, &senderArgs);
    pthread_create(&receiverThread, nullptr, network_receiver_thread, &receiverArgs);
    pthread_create(&mixerThreadId, nullptr, mixer_thread, &mixerArgs);
    pthread_create(&lowPassThread, nullptr, low_pass_thread, &lowPassArgs);
    pthread_create(&echoThread, nullptr, echo_cancellation_thread, &echoArgs);
    pthread_create(&finalMixThreadId, nullptr, final_mix_thread, &finalMixArgs);

    std::cout << "Conferencing started..." << std::endl;

    pthread_join(recorderThread, nullptr);
    pthread_join(playerThread, nullptr);
    pthread_join(senderThread, nullptr);
    pthread_join(receiverThread, nullptr);
    pthread_join(mixerThreadId, nullptr);
    pthread_join(lowPassThread, nullptr);
    pthread_join(echoThread, nullptr);
    pthread_join(finalMixThreadId, nullptr);

    Pa_Terminate();

    return 0;
}