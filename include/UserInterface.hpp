#pragma once

#include <string>

/**
 * @class UserInterface
 * @brief Simple GPIO user interface for reading a button and writing an LED.
 *
 * This class uses the Linux sysfs GPIO interface.
 */
class UserInterface {
private:
    int buttonGpio;
    int ledGpio;

    void exportGpio(int gpio);
    void unexportGpio(int gpio);
    void setDirection(int gpio, const std::string& direction);
    void writeGpio(int gpio, int value);
    int readGpio(int gpio);

public:
    /**
     * @brief Constructs the user interface.
     *
     * @param buttonGpio GPIO number connected to the button.
     * @param ledGpio GPIO number connected to the LED.
     */
    UserInterface(int buttonGpio, int ledGpio);

    /**
     * @brief Destroys the user interface and unexports GPIOs.
     */
    ~UserInterface();

    /**
     * @brief Reads the button state.
     *
     * @return true if button is pressed, false otherwise.
     */
    bool isButtonPressed();

    /**
     * @brief Sets the LED state.
     *
     * @param on true turns LED on, false turns LED off.
     */
    void setLed(bool on);
};