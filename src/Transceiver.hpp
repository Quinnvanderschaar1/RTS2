#pragma once

#include "AudioFifo.hpp"
#include "UdpSender.hpp"
#include "UdpReceiver.hpp"
#include "wcet.hpp"

#include <functional>

void transmitLoop(
    AudioFifo& micFifo,
    AudioFifo& playbackFifo,
    const std::function<bool()>& isActive,
    const std::function<void(bool)>& setLed,
    UdpSender* sender,
    bool simulationMode
);

void receiveLoop(AudioFifo& playbackFifo, UdpReceiver& receiver);
