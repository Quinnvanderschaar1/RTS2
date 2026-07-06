#pragma once

// Legacy/global frame divisor used by older code paths.
extern int gFrameDivisor;

// Total number of audio frames in one full audio buffer for a given audio block duration.
// Example: 48 kHz * 10 ms = 480 frames.
extern int gFramesPerBuffer;

// Capacity of the FIFO queues between pipeline stages.
extern int FIFO_SIZE;

// Optional loop size used by the heavy-loop benchmark path.
extern int loop_size;

// Audio block duration in milliseconds.
// This is converted into frames using the sample rate in Application.cpp.
extern int gAudioMs;

// How many smaller processing blocks one full buffer is split into.
// Example: if gFramesPerBuffer is 480 and gSplitDivisor is 10, each processing block is 48 frames.
extern int gSplitDivisor;

// Number of frames handled by each processing block.
// Computed as gFramesPerBuffer / gSplitDivisor.
extern int gProcessFrames;

// How many processing blocks are grouped together into one outgoing packet.
// In the current setup it is set equal to gSplitDivisor, so one full buffer becomes one packet.
extern int gBlocksPerPacket;