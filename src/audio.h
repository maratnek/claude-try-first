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
    Sound boomSound{};  // rebuilt per crash, its length depends on the explosion scale
};

// Synthesizes a looping engine-drone waveform (no external audio files).
// Call once after InitAudioDevice().
void LoadEngineAudio(EngineAudio &audio);

// Restarts the loop if it ended, and adjusts pitch/volume from 0..1 engine
// power so the engine sounds busier at higher power. With power at zero the
// loop drops to a faint low windmilling rumble scaled by 0..1 speed fraction,
// and stops once the plane is nearly still.
void UpdateEngineAudio(EngineAudio &audio, float power, float speedFraction);

// Silences the engine loop (crash, menu); UpdateEngineAudio restarts it.
void StopEngineAudio(EngineAudio &audio);

// One-shot impact + crunch, plus a low boom whose gain and length grow with
// the explosion scale s (0..1, from fuel). Below 0.05 there is no boom.
void PlayCrashSound(EngineAudio &audio, float s);

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
