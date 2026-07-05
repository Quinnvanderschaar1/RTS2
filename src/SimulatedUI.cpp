#include "SimulatedUI.hpp"
#include "TimingLogger.hpp"
#include <cstdlib>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <iostream>

namespace {
void saveAndExit() {
    gTimingLogger.saveCSV("timing_report.csv");
    gTimingLogger.saveFifoCSV("fifo_occupancy.csv");
    gTimingLogger.saveQueueCSV("queue_latency.csv");
    gTimingLogger.saveNetworkCSV("network_jitter.csv");
    gTimingLogger.saveDropCSV("dropped_frames.csv");
    std::exit(0);
}
}

static bool isKeyboardPressedOnce() {
    termios oldt{};
    termios newt{};

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    int oldFlags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldFlags | O_NONBLOCK);

    int ch = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldFlags);

    if (ch == 'x' || ch == 'X') {
        std::cout << "Saving CSV files and exiting..." << std::endl;
        saveAndExit();
    }

    return ch == ' ';
}

bool SimulatedUserInterface::isButtonPressed() {
    if (isKeyboardPressedOnce()) {
        keyboardEnabled = !keyboardEnabled;
        printf("Keyboard toggle d: %s\n", keyboardEnabled ? "ON" : "OFF");
        fflush(stdout);
    }
    return keyboardEnabled;
}

void SimulatedUserInterface::setLed(bool) {
    // no-op for simulation
}
