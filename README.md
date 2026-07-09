# RTS2
Real time conferencing application

## Pipeline walkthrough

A detailed explanation of the audio path, packet formation, and the meaning of the global variables is available in [README_PIPELINE.md](README_PIPELINE.md).

The short version is:

```text
microphone / simulator
    -> recorder
    -> low-pass filter
    -> echo cancellation
    -> audio encoding
    -> packet builder / UDP transmit
    -> receiver
    -> audio decoding
    -> playback
```

The system does not move one sample at a time. It works in blocks:

- one full audio buffer is split into smaller processing blocks
- each processing block is handled by the pipeline stages
- several small blocks are grouped into one outgoing packet
- the receiver splits that packet back into small blocks for decoding and playback

### Example
If the audio buffer is 480 frames and the split divisor is 10, then each processing block contains 48 frames. Ten of those blocks are grouped into one packet, so the packet carries 480 frames in total.

# Documentation
install:
```
sudo apt install doxygen graphviz
```


## installs
```
sudo apt install portaudio19-dev

sudo add-apt-repository ppa:ubuntu-toolchain-r/test
sudo apt update
sudo apt install libstdc++6
```




## build
Install cross compilers:
```
sudo apt install mingw-w64
sudo apt install g++-aarch64-linux-gnu
```

Build program:
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

Analyze cpu usage:
```
htop
top -H -p $(pidof conferencing)
```

## analysis

sudo prlimit --rtprio=99 --pid=$$
sudo setcap cap_sys_nice+ep ./conferencing
permissions:
```
sudo prlimit --rtprio=99 --pid=$$
sudo mount -o remount,mode=755 /sys/kernel/tracing/
sudo chmod -R a+rX /sys/kernel/tracing/events/sched
sudo sysctl kernel.perf_event_paranoid=-1
```

generate and visualize plot
```
perf sched record -- ./conferencing 0.0.0.0 --div 200 --fifo 8
hotspot perf.data
```

sudo trace-cmd record -e sched_switch -e sched_wakeup --   sudo -E -u "$USER" env   XDG_RUNTIME_DIR="$XDG_RUNTIME_DIR"   PULSE_SERVER="$PULSE_SERVER"   DBUS_SESSION_BUS_ADDRESS="$DBUS_SESSION_BUS_ADDRESS"   ./conferencing 0.0.0.0 --ms 10 --split 20 --fifo 2000

kernelshark trace.dat

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
cpack -G DEB
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

