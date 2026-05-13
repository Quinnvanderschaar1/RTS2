#include "UserInterface.hpp"

#include <fstream>
#include <string>
#include <unistd.h>

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

bool UserInterface::isButtonPressed() {
    return readGpio(buttonGpio) == 1;
}

void UserInterface::setLed(bool on) {
    writeGpio(ledGpio, on ? 1 : 0);
}