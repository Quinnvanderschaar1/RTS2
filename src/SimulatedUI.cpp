#include "SimulatedUI.hpp"
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <iostream>

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

    return ch == ' ';
}

bool SimulatedUserInterface::isButtonPressed() {
    if (isKeyboardPressedOnce()) {
        keyboardEnabled = !keyboardEnabled;
        printf("Keyboard toggle: %s\n", keyboardEnabled ? "ON" : "OFF");
        fflush(stdout);
    }
    return keyboardEnabled;
}

void SimulatedUserInterface::setLed(bool) {
    // no-op for simulation
}
