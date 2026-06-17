#pragma once

class SimulatedUserInterface {
private:
    bool keyboardEnabled = true;
public:
    bool isButtonPressed();
    void setLed(bool on);
};
