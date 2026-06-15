# RTS2
Real time conferencing application



## installs
```
sudo apt install portaudio19-dev

sudo add-apt-repository ppa:ubuntu-toolchain-r/test
sudo apt update
sudo apt install libstdc++6
```




## build
```
sudo apt install mingw-w64
sudo apt install g++-aarch64-linux-gnu
```


```
mkdir build
cd build
cmake ..
make
cpack
```

## Running
```
./conferencing ip_address_other_device
```

# RTS2 Real-Time Conferencing System

A real-time conferencing system developed for the Real-Time Systems 2 course.

The project models and implements a low-latency audio conferencing pipeline using:
- HSDF-inspired dataflow architecture
- FIFO-based communication
- real-time audio processing
- UDP communication between nodes
- simulation and hardware execution modes

---

# Features

## Current Features
- Simulated audio input/output
- FIFO-based data-driven pipeline
- UDP transmission/reception
- Noise filtering / audio processing framework
- Multi-threaded architecture
- WCET measurement utilities
- Simulation mode without Raspberry Pi hardware

## Planned Features
- Echo cancellation
- Packet loss concealment
- Clock synchronization
- Multi-node conferencing
- Real-time scheduling optimization
- Raspberry Pi deployment

---

# System Architecture

The conferencing system follows a data-driven HSDF-style pipeline:

```text
Audio Source
    ↓
Input FIFO
    ↓
Audio Processing
    ↓
UDP Transmission
    ↓
UDP Reception
    ↓
Playback FIFO
    ↓
Audio Output
```

---

# Timing Model

## Audio Configuration

| Parameter | Value |
|---|---|
| Sample Rate | 48000 Hz |
| Channels | 1 |
| Frame Duration | 10 ms |
| Samples per Frame | 480 |

---

# Real-Time Goals

| Goal | Target |
|---|---|
| End-to-End Latency | < 20 ms |
| Throughput | ≥ 100 frames/sec |
| Architecture | Data-driven |

---

# Estimated Timing Analysis

## WCET Estimation

| Component | WCET |
|---|---|
| Audio Capture | 0.5 ms |
| FIFO Access | 0.05 ms |
| Audio Processing | 2.0 ms |
| UDP Transmission | 0.3 ms |
| UDP Reception | 0.3 ms |
| Playback | 1.5 ms |

### Total WCET

```math
WCET_{total} \approx 4.9 \text{ ms}
```

### End-to-End Latency

```math
Latency_{total}
=
Latency_{frame}
+
WCET_{total}
```

```math
Latency_{total}
=
10 + 4.9
=
14.9 \text{ ms}
```

---

# Build Instructions

## Requirements

- CMake
- C++17
- pthread
- PortAudio (hardware mode only)

Ubuntu / WSL:

```bash
sudo apt update
sudo apt install cmake g++ portaudio19-dev
```

---

# Build

## Simulation Mode

Simulation mode does not require audio hardware.

```bash
mkdir build
cd build
cmake -DUSE_SIMULATION=ON ..
make
```

Run:

```bash
./conferencing
```

---

## Hardware Mode

Hardware mode uses:
- microphone
- speaker
- UDP networking
- PortAudio

```bash
mkdir build
cd build
cmake ..
make
```

Run:

```bash
./conferencing --hw
```

Or specify a UDP address:

```bash
./conferencing 192.168.50.189
```

---

# Runtime Modes

## Simulation Mode

Features:
- simulated microphone
- simulated playback
- keyboard-controlled transmit toggle

Controls:

| Key | Action |
|---|---|
| SPACE | Toggle transmission |

---

## Hardware Mode

Uses:
- PortAudio
- GPIO button
- GPIO LED
- UDP multicast communication

---

# Thread Architecture

The system is implemented as a multi-threaded real-time pipeline.

| Thread | Responsibility |
|---|---|
| Recorder Thread | Audio capture |
| Player Thread | Audio playback |
| Transmit Thread | Processing + UDP sending |
| Receive Thread | UDP reception |

---

# Repository Structure

```text
include/
    AudioFifo.hpp
    AudioProcessing.hpp
    AudioMixer.hpp
    UdpSender.hpp
    UdpReceiver.hpp
    wcet.hpp

src/
    AudioRecorder.cpp
    AudioPlayer.cpp
    AudioProcessing.cpp
    AudioMixer.cpp
    UdpSender.cpp
    UdpReceiver.cpp

main/
    main.cpp
```

---

# WCET Measurement

Execution times are measured using:

```cpp
std::chrono::high_resolution_clock
```

Measured values include:
- processing latency
- FIFO delays
- transmission timing
- end-to-end latency

---

# Future Work

- Echo cancellation
- Ethernet synchronization
- Packet loss concealment
- Real-time Linux scheduling
- Adaptive jitter buffering
- Multi-user conferencing

---