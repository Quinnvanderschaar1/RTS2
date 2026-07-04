# Audio pipeline and packet flow

This document explains how audio moves through the conferencing application, how it is split into smaller blocks, and how those blocks are grouped into UDP packets.

## 1. Big picture

The pipeline is:

```text
Microphone / simulation source
    -> recorder
    -> low-pass filter
    -> echo cancellation
    -> audio encoding
    -> transmit / packet builder
    -> UDP sender or local loopback
    -> receiver
    -> audio decoding
    -> playback
```

In the code, this is implemented with several FIFO queues:

- micFifo: data coming from the recorder
- lowPassFifo: data after low-pass filtering
- echoCancelFifo: data after echo cancellation
- audioEncoderFifo: data after encoding
- audioDecoderFifo: data after decoding
- playbackFifo: data waiting for playback

## 2. What is an audio block?

The application does not move raw audio samples one-by-one. It moves them in small chunks called AudioBlock objects.

Each AudioBlock contains:

- captureNs: capture timestamp
- pushNs: push timestamp
- sendNs: send timestamp
- samples: the actual audio samples for that block

The important part is the `samples` vector. That is the actual audio data.

## 3. How the system is split into blocks

The system uses two levels of splitting:

1. A callback buffer is created for one audio chunk.
2. That buffer is split into smaller processing blocks.

The key values come from the globals in [include/Globals.hpp](include/Globals.hpp):

- gFramesPerBuffer: total number of audio frames in one full audio buffer
- gProcessFrames: number of frames in each smaller processing block
- gSplitDivisor: how many smaller processing blocks are created from one full buffer
- gBlocksPerPacket: how many processing blocks are packed into one outgoing packet

The setup logic in [src/Application.cpp](src/Application.cpp) does this:

```cpp
gFramesPerBuffer = SAMPLE_RATE * gAudioMs / 1000;
gProcessFrames = gFramesPerBuffer / gSplitDivisor;
gBlocksPerPacket = gSplitDivisor;
```

So the full buffer is always divided into `gSplitDivisor` equal pieces.

## 4. The flow step by step

### 4.1 Source -> recorder

The recorder thread reads audio from the microphone (or from the simulator in simulation mode).

For each callback buffer, it creates one or more AudioBlock objects.

Each block contains only `gProcessFrames` samples, not the whole buffer.

Example:

- audio block size = 10 ms
- sample rate = 48000 Hz
- total frames in one buffer = 480
- split divisor = 10
- each processing block contains 48 frames

So one full 10 ms audio chunk is split into 10 blocks of 48 samples each.

### 4.2 Recorder -> low-pass filter

The recorder pushes these small blocks into the mic FIFO.

The low-pass thread pops one block, applies the low-pass filter, and pushes the result to the next FIFO.

This is done block-by-block, not on the entire original buffer at once.

### 4.3 Low-pass -> echo cancellation

The echo-cancellation stage receives the filtered blocks and removes a delayed version of the signal.

Again, the block is processed as one small chunk.

### 4.4 Echo cancellation -> encoding

The encoder applies an audio encoding transformation to each block.

This makes the data less direct and prepares it for transmission.

### 4.5 Encoding -> transmit / packet builder

This is the step where packets are formed.

The transmit loop collects `gBlocksPerPacket` small blocks from the encoder FIFO and combines them into one larger packet-like AudioBlock.

That means:

- each small block is still a normal processing block
- the packet block is a larger container that holds many small blocks concatenated together

The packet block is then either:

- sent over UDP, or
- pushed to the local playback path in simulation mode

### 4.6 Receive -> decoding

On the receiving side, the receiver gets the packet payload and splits it back into smaller blocks.

Each block is then pushed into the decoder FIFO.

### 4.7 Decoding -> playback

The decoder reverses the encoding and pushes the decoded blocks into the playback FIFO.

The player thread consumes them and sends them to the audio output.

## 5. How packets are formed

The important rule is:

- one full audio buffer is split into `gSplitDivisor` sub-blocks
- the transmitter groups `gBlocksPerPacket` sub-blocks into one outgoing packet

In the current code, these two values are set equal:

```cpp
gProcessFrames = gFramesPerBuffer / gSplitDivisor;
gBlocksPerPacket = gSplitDivisor;
```

That means the transmitter generally packs one full audio buffer worth of data into one network packet.

### Packet structure in practice

If:

- one full buffer has 480 samples
- split divisor = 10
- each processing block has 48 samples

then:

- 10 small blocks are created from one full buffer
- 10 small blocks are packed into one packet
- the packet contains 480 samples total

So a packet is not a single tiny audio chunk. It is a bundle of many small processing blocks.

## 6. Example with numbers

Assume:

- sample rate = 48000 Hz
- audio block duration = 10 ms
- split divisor = 10

### Step A: compute full buffer size

```text
gFramesPerBuffer = 48000 * 10 / 1000 = 480 frames
```

### Step B: compute processing block size

```text
gProcessFrames = 480 / 10 = 48 frames
```

### Step C: create the blocks

The recorder creates 10 blocks:

- block 1: 48 samples
- block 2: 48 samples
- ...
- block 10: 48 samples

### Step D: form one packet

The transmit loop takes those 10 blocks and combines them into one packet payload containing:

```text
10 * 48 = 480 samples
```

That packet is then sent over UDP.

### Step E: receive and split again

On the receiver side, the packet is split back into the same 10 smaller blocks of 48 samples each, then passed to the decoder and playback pipeline.

## 7. Why the variables matter

These variables control how much data is processed at a time and how much data is grouped together:

- larger gFramesPerBuffer means larger audio chunks
- larger gSplitDivisor means more smaller blocks per full buffer
- larger gProcessFrames means fewer blocks per full buffer
- larger gBlocksPerPacket means more blocks per packet

The current implementation is designed so that the full audio buffer is divided into many small processing blocks, and those blocks are reassembled into a packet for transmission.

## 8. Short mental model

A simple way to think about it:

- the recorder makes many small pieces of audio
- each stage processes one piece
- the transmitter gathers many pieces into one packet
- the receiver splits the packet back into pieces
- the player plays the pieces in order

## 9. Practical takeaway

If you want to understand the system quickly, remember this rule:

```text
full buffer -> many small processing blocks -> one packet -> many small processing blocks again
```

That is the core of the packet flow in this project.
