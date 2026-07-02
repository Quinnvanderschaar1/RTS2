#pragma once

#include "ProgramOptions.hpp"

/**
 * @class Application
 * @brief Main conferencing application controller.
 *
 * The Application class owns the high-level runtime flow of the program:
 * audio configuration, startup logging, device/simulation setup, thread
 * creation, thread joining, and shutdown cleanup.
 */
class Application {
public:
    /**
     * @brief Constructs the application with parsed program options.
     *
     * @param options Runtime configuration parsed from command-line arguments.
     */
    explicit Application(ProgramOptions options);

    /**
     * @brief Runs the conferencing application.
     *
     * @return 0 on successful termination, non-zero on startup/configuration error.
     */
    int run();

private:
    /**
     * @brief Parsed runtime options used by the application.
     */
    ProgramOptions options;

    /**
     * @brief Configures global audio parameters from program options.
     *
     * Sets values such as audio block size, frames per buffer, process frames,
     * and blocks per packet.
     *
     * @return true if audio configuration is valid, false otherwise.
     */
    bool configureAudio();

    /**
     * @brief Registers handlers that save timing and diagnostic CSV files on exit.
     */
    void registerExitHandlers();

    /**
     * @brief Prints startup configuration and usage information.
     *
     * @param echoDelaySamples Echo cancellation delay in audio samples.
     */
    void printStartupInfo(int echoDelaySamples) const;
};