#include "AudioFifo.hpp"
#include "AudioRecorder.hpp"
#include "AudioPlayer.hpp"

#include <pthread.h>
#include <portaudio.h>

void* recorder_thread(void* arg) {
    static_cast<AudioRecorder*>(arg)->start();
    return nullptr;
}

void* player_thread(void* arg) {
    static_cast<AudioPlayer*>(arg)->start();
    return nullptr;
}

int main() {
    Pa_Initialize();

    AudioFifo fifo;
    AudioRecorder recorder(fifo);
    AudioPlayer player(fifo);

    pthread_t recorderThread;
    pthread_t playerThread;

    pthread_create(&recorderThread, nullptr, recorder_thread, &recorder);
    pthread_create(&playerThread, nullptr, player_thread, &player);

    pthread_join(recorderThread, nullptr);
    pthread_join(playerThread, nullptr);

    Pa_Terminate();

    return 0;
}