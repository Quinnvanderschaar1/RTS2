#include "AudioFifo.hpp"

AudioFifo::AudioFifo() {
    pthread_mutex_init(&mutex, nullptr);
    pthread_cond_init(&cond, nullptr);
}

AudioFifo::~AudioFifo() {
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);
}

void AudioFifo::push(const AudioBlock& block) {
    pthread_mutex_lock(&mutex);
    queue.push(block);
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);
}

AudioBlock AudioFifo::pop() {
    pthread_mutex_lock(&mutex);

    while (queue.empty()) {
        pthread_cond_wait(&cond, &mutex);
    }

    AudioBlock block = queue.front();
    queue.pop();

    pthread_mutex_unlock(&mutex);

    return block;
}