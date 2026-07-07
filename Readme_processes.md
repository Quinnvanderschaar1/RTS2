# Audio processing flow

This document describes how the audio processing steps in this project are implemented in the current code.

## 1. Low-pass filtering

The low-pass filter is implemented in the function `AudioProcessing::lowPass` in [src/AudioProcessing.cpp](src/AudioProcessing.cpp).

### What it does
- It processes the input audio samples one by one.
- The first output sample is copied directly from the first input sample.
- Every following sample is computed with this first-order IIR formula:

  $$y[i] = \alpha \cdot x[i] + (1 - \alpha) \cdot y[i-1]$$

- `alpha` is the smoothing factor:
  - larger values make the filter respond more quickly,
  - smaller values make the filtering stronger and smoother.

### Implementation details
- If the input vector is empty, the function returns an empty vector.
- The output vector has the same size as the input vector.
- The filter uses the previous filtered sample as memory, which is the classic recursive low-pass behavior.

### Where it is used
- The function is called inside the low-pass processing thread in [src/ProcessingThreads.cpp](src/ProcessingThreads.cpp).
- That thread reads an `AudioBlock` from an input FIFO, applies the filter, and pushes the result to the next FIFO.

---

## 2. Echo cancellation

The echo cancellation step is implemented in `AudioProcessing::echoCancellation` in [src/AudioProcessing.cpp](src/AudioProcessing.cpp).

### What it does
- For each sample, the code estimates a delayed echo contribution.
- If the current sample index is at least `delaySamples`, it uses a previously produced output sample from `delaySamples` steps earlier.
- That delayed sample is multiplied by `decay` and subtracted from the current input sample.

The processing is essentially:

$$echo = output[i - delaySamples] \cdot decay$$

$$output[i] = input[i] - echo$$

### Implementation details
- The delayed echo is only considered when the sample index is large enough.
- After subtraction, the output is clamped to the range `[-1.0, 1.0]`.
- This prevents the processed signal from exceeding the expected audio amplitude bounds.

### Where it is used
- The function is called in the echo-cancellation thread in [src/ProcessingThreads.cpp](src/ProcessingThreads.cpp).
- The thread pops a block, applies the cancellation, and pushes the updated block forward.

---

## 3. Audio encoding

The audio encoding step is implemented in `AudioProcessing::audioEncoding` in [src/AudioProcessing.cpp](src/AudioProcessing.cpp).

### What it does
- Each input sample is transformed using a simple mu-law style encoding formula.
- The implementation preserves the sign of the sample and applies the transformation to its absolute value.

The encoding formula used is:

$$encoded = sign(x) \cdot \frac{\ln(1 + \mu \cdot |x|)}{\ln(1 + \mu)}$$

with:

- `mu = 255.0`
- `sign(x) = -1` for negative values and `+1` for non-negative values

### Implementation details
- The absolute value of the input is used for the logarithmic compression.
- The result is stored in the output vector with the same sign as the input sample.

### Where it is used
- The encoder thread in [src/ProcessingThreads.cpp](src/ProcessingThreads.cpp) calls this function on each `AudioBlock`.

---

## 4. Audio decoding

The audio decoding step is implemented in `AudioProcessing::audioDecoding` in [src/AudioProcessing.cpp](src/AudioProcessing.cpp).

### What it does
- It performs the inverse of the encoding step.
- The implementation restores the sign and applies the inverse mu-law transformation.

The decoding formula used is:

$$decoded = sign(x) \cdot \frac{(1 + \mu)^{|x|} - 1}{\mu}$$

with:

- `mu = 255.0`

### Implementation details
- Like the encoder, it uses the sign of each sample and applies the nonlinear inverse transform to the absolute value.
- The output is written to a new vector of the same size as the input.

### Where it is used
- The decoder thread in [src/ProcessingThreads.cpp](src/ProcessingThreads.cpp) calls this function for each `AudioBlock`.

---

## 5. How the processing stages are executed

The current implementation does not chain all four steps inside a single function. Instead, each processing stage runs in its own worker thread:

1. low-pass filtering thread
2. echo-cancellation thread
3. audio-encoding thread
4. audio-decoding thread

Each thread follows the same pattern:
- read one `AudioBlock` from an input FIFO,
- apply the processing function,
- push the modified block into an output FIFO.

This makes each stage independent and allows the pipeline to be processed in a threaded manner.

---

## 6. Summary

In short, the current audio processing pipeline is:
- low-pass filtering using a recursive first-order filter,
- echo cancellation by subtracting a delayed and decayed version of the signal,
- mu-law style encoding using a logarithmic transform,
- mu-law style decoding using the inverse exponential transform.

These operations are implemented as simple numerical transforms and are executed in dedicated processing threads.
