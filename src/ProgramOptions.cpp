#include "ProgramOptions.hpp"

#include <cstdlib>
#include <string>

extern int FIFO_SIZE;
extern int loop_size;

namespace {

bool hasPrefix(const std::string& value, const std::string& prefix)
{
    return value.size() >= prefix.size() &&
           value.compare(0, prefix.size(), prefix) == 0;
}

}

ProgramOptions parseArguments(int argc, char* argv[])
{
    ProgramOptions options;

    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];

        if (arg == "--hw")
            options.simulationMode = false;

        else if (arg == "--sim")
            options.simulationMode = true;

        else if (arg == "--no-udp")
            options.useUdp = false;

        else if (arg == "--np")
            options.useProcessing = false;

        else if (arg == "--ms" && i + 1 < argc)
            options.audioMs = std::atoi(argv[++i]);

        else if (arg == "--split" && i + 1 < argc)
            options.splitDivisor = std::atoi(argv[++i]);

        else if (arg == "--fifo" && i + 1 < argc)
            FIFO_SIZE = std::atoi(argv[++i]);

        else if (arg == "--loop" && i + 1 < argc)
            loop_size = std::atoi(argv[++i]);

        else if (!hasPrefix(arg, "--"))
        {
            options.udpGroup = arg;
            options.userSpecifiedUdp = true;
            options.simulationMode = false;
        }
    }

    return options;
}