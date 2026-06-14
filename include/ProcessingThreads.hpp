#pragma once

#include "AudioFifo.hpp"
#include "AudioProcessing.hpp"

/**
 * @brief Runs the low-pass processing thread.
 *
 * This function blocks forever and logs timing for each pop/process/push step.
 */
void lowPassThreadLoop(
    AudioFifo& source,
    AudioFifo& destination,
    AudioProcessing& processing,
    float alpha
);

/**
 * @brief Runs the echo cancellation processing thread.
 *
 * This function blocks forever and logs timing for each pop/process/push step.
 */
void echoCancelThreadLoop(
    AudioFifo& source,
    AudioFifo& destination,
    AudioProcessing& processing,
    int delaySamples,
    float decay
);
