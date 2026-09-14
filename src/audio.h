#pragma once
#include "raylib.h"

struct EngineAudio {
    Sound engineSound{};
};

// Synthesizes a looping engine-drone waveform (no external audio files).
// Call once after InitAudioDevice().
void LoadEngineAudio(EngineAudio &audio);

// Restarts the loop if it ended, and adjusts pitch/volume from 0..1 speed
// fraction so the engine sounds busier at higher throttle.
void UpdateEngineAudio(EngineAudio &audio, float speedFraction);

void UnloadEngineAudio(EngineAudio &audio);
