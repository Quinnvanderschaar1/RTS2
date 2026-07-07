#include "UserInterface.hpp"
#include "TimingLogger.hpp"

#include <fstream>
#include <string>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <stdio.h>

UserInterface::UserInterface(int buttonGpio, int ledGpio)
    : buttonGpio(buttonGpio), ledGpio(ledGpio)
{
    exportGpio(buttonGpio);
    exportGpio(ledGpio);

    usleep(100000);

    setDirection(buttonGpio, "in");
    setDirection(ledGpio, "out");
}

UserInterface::~UserInterface() {
    unexportGpio(buttonGpio);
    unexportGpio(ledGpio);
}

void UserInterface::exportGpio(int gpio) {
    std::ofstream file("/sys/class/gpio/export");
    file << gpio;
}

void UserInterface::unexportGpio(int gpio) {
    std::ofstream file("/sys/class/gpio/unexport");
    file << gpio;
}

void UserInterface::setDirection(int gpio, const std::string& direction) {
    std::ofstream file(
        "/sys/class/gpio/gpio" + std::to_string(gpio) + "/direction"
    );

    file << direction;
}

void UserInterface::writeGpio(int gpio, int value) {
    std::ofstream file(
        "/sys/class/gpio/gpio" + std::to_string(gpio) + "/value"
    );

    file << value;
}

int UserInterface::readGpio(int gpio) {
    std::ifstream file(
        "/sys/class/gpio/gpio" + std::to_string(gpio) + "/value"
    );

    int value = 0;
    file >> value;

    return value;
}

bool UserInterface::isKeyboardPressed() {
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
        printf("Saving CSV files and exiting...\n");
        gTimingLogger.saveCSV("timing_report.csv");
        gTimingLogger.saveFifoCSV("fifo_occupancy.csv");
        gTimingLogger.saveQueueCSV("queue_latency.csv");
        gTimingLogger.saveNetworkCSV("network_jitter.csv");
        gTimingLogger.saveDropCSV("dropped_frames.csv");
        std::exit(0);
    }

    return ch == ' ';
}

bool UserInterface::isButtonPressed() {
    if (isKeyboardPressed()) {
        keyboardEnabled = !keyboardEnabled;

        printf("Keyboard toggle: %s\n", keyboardEnabled ? "ON" : "OFF");
        fflush(stdout);
    }

    return readGpio(buttonGpio) == 1 || keyboardEnabled;
}

void UserInterface::setLed(bool on) {
    writeGpio(ledGpio, on ? 1 : 0);
}