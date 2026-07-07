#pragma once

#include <string>

struct ProgramOptions {
    bool simulationMode = false;
    std::string udpGroup = "192.168.50.189";
    bool useUdp = true;
    bool userSpecifiedUdp = false;
    bool useProcessing = true;

    int audioMs = 10;
    int splitDivisor = 10;
};

ProgramOptions parseArguments(int argc, char* argv[]);