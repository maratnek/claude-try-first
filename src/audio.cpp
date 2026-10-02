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

    constexpr float kCrashSeconds = 1.4f;
    int crashFrames = (int)(kSampleRate * kCrashSeconds);
    std::vector<short> crash(crashFrames);
    unsigned int rng = 12345u;
    float lowpass = 0.0f;
    float thumpPhase = 0.0f;
    for (int i = 0; i < crashFrames; i++) {
        float t = (float)i / (float)kSampleRate;
        rng = rng * 1664525u + 1013904223u;
        float noise = ((rng >> 8) & 0xFFFF) / 32768.0f - 1.0f;
        lowpass += 0.25f * (noise - lowpass);

        float thumpFreq = 40.0f + 80.0f * expf(-t * 12.0f);
        thumpPhase += 2.0f * PI * thumpFreq / (float)kSampleRate;
        float thump = sinf(thumpPhase) * expf(-t * 5.0f);

        float burst = noise * expf(-t * 30.0f);
        float rumble = lowpass * expf(-t * 4.0f);
        float crunch = (fabsf(noise) > 0.85f ? noise : 0.0f) * expf(-t * 6.0f);

        float value = 0.6f * thump + 0.5f * burst + 0.7f * rumble + 0.5f * crunch;
        crash[i] = (short)(Clamp(value, -1.0f, 1.0f) * 28000.0f);
    }
    wave.frameCount = crashFrames;
    wave.data = crash.data();
    audio.crashSound = LoadSoundFromWave(wave);
}

void StopEngineAudio(EngineAudio &audio) {
    if (IsSoundPlaying(audio.engineSound)) StopSound(audio.engineSound);
}

void PlayCrashSound(EngineAudio &audio) {
    PlaySound(audio.crashSound);
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
    UnloadSound(audio.crashSound);
}
