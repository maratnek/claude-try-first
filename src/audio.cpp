#include "audio.h"
#include "raymath.h"
#include <cmath>
#include <vector>

namespace {
constexpr int kSampleRate = 44100;
constexpr float kDurationSeconds = 1.0f;
constexpr float kBaseFreq = 90.0f;
}  // namespace

void LoadEngineAudio(EngineAudio &audio) {
    int frameCount = (int)(kSampleRate * kDurationSeconds);
    std::vector<short> samples(frameCount);

    // Layer the fundamental with a couple of harmonics for a buzzy,
    // engine-like drone. kDurationSeconds * kBaseFreq is a whole number of
    // cycles, so the buffer loops without an audible click.
    for (int i = 0; i < frameCount; i++) {
        float t = (float)i / (float)kSampleRate;
        float value = 0.5f * sinf(2.0f * PI * kBaseFreq * t) +
                      0.3f * sinf(2.0f * PI * kBaseFreq * 2.0f * t) +
                      0.2f * sinf(2.0f * PI * kBaseFreq * 3.0f * t);
        samples[i] = (short)(value * 12000.0f);
    }

    Wave wave{};
    wave.frameCount = frameCount;
    wave.sampleRate = kSampleRate;
    wave.sampleSize = 16;
    wave.channels = 1;
    wave.data = samples.data();

    audio.engineSound = LoadSoundFromWave(wave);
}

void UpdateEngineAudio(EngineAudio &audio, float speedFraction) {
    float t = Clamp(speedFraction, 0.0f, 1.0f);

    if (!IsSoundPlaying(audio.engineSound)) {
        PlaySound(audio.engineSound);
    }
    SetSoundPitch(audio.engineSound, 0.7f + t * 0.8f);
    SetSoundVolume(audio.engineSound, 0.35f + t * 0.35f);
}

void UnloadEngineAudio(EngineAudio &audio) {
    UnloadSound(audio.engineSound);
}
