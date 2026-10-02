#pragma once
#include "raylib.h"

struct EngineAudio {
    Sound engineSound{};
    Sound crashSound{};
};

// Synthesizes a looping engine-drone waveform (no external audio files).
// Call once after InitAudioDevice().
void LoadEngineAudio(EngineAudio &audio);

// Restarts the loop if it ended, and adjusts pitch/volume from 0..1 speed
// fraction so the engine sounds busier at higher throttle.
void UpdateEngineAudio(EngineAudio &audio, float speedFraction);

// Silences the engine loop (crash, menu); UpdateEngineAudio restarts it.
void StopEngineAudio(EngineAudio &audio);

// One-shot impact + crunch.
void PlayCrashSound(EngineAudio &audio);

void UnloadEngineAudio(EngineAudio &audio);
