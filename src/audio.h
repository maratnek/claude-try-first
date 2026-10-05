#pragma once
#include "raylib.h"

struct EngineAudio {
    Sound engineSound{};
    Sound crashSound{};
    Sound chimeSound{};
    Sound windSound{};
    Sound touchdownSound{};
    Sound clickSound{};
    Sound damageSound{};
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

// One-shot two-note chime for a checkpoint ring.
void PlayChimeSound(EngineAudio &audio);

// One-shot low thump for a safe touchdown.
void PlayTouchdownSound(EngineAudio &audio);

void PlayClickSound(EngineAudio &audio);

void PlayDamageSound(EngineAudio &audio);

float SetMasterVolumeClamped(float volume);

// Looped wind noise; volume and pitch rise with 0..1 speed fraction, much
// quieter on the ground, and it is stopped entirely when near-silent.
void UpdateWindAudio(EngineAudio &audio, float speedFraction, bool airborne);

// Silences the wind loop (menu, crash, finish).
void StopWindAudio(EngineAudio &audio);

// Cuts the one-shots (chime, touchdown, crash, damage) so a restart starts quiet.
void StopOneShotSounds(EngineAudio &audio);

void UnloadEngineAudio(EngineAudio &audio);
