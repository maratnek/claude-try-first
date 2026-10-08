#include "audio.h"
#include "raymath.h"
#include <cmath>
#include <vector>

namespace {
constexpr int kSampleRate = 44100;
constexpr float kDurationSeconds = 1.0f;
constexpr float kBaseFreq = 90.0f;
constexpr float kNoBoomBelow = 0.05f;
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

    constexpr float kChimeSeconds = 0.7f;
    constexpr float kChimeNoteFreqs[2] = {880.0f, 1318.5f};
    constexpr float kChimeSecondNoteStart = 0.12f;
    int chimeFrames = (int)(kSampleRate * kChimeSeconds);
    std::vector<short> chime(chimeFrames);
    for (int i = 0; i < chimeFrames; i++) {
        float t = (float)i / (float)kSampleRate;
        float value = 0.0f;
        for (int n = 0; n < 2; n++) {
            float nt = t - (n == 0 ? 0.0f : kChimeSecondNoteStart);
            if (nt < 0.0f) continue;
            float attack = Clamp(nt / 0.005f, 0.0f, 1.0f);
            float tone = sinf(2.0f * PI * kChimeNoteFreqs[n] * nt) +
                         0.25f * sinf(2.0f * PI * kChimeNoteFreqs[n] * 2.0f * nt);
            value += 0.4f * tone * attack * expf(-nt * 6.0f);
        }
        chime[i] = (short)(Clamp(value, -1.0f, 1.0f) * 16000.0f);
    }
    wave.frameCount = chimeFrames;
    wave.data = chime.data();
    audio.chimeSound = LoadSoundFromWave(wave);

    constexpr float kTouchdownSeconds = 0.35f;
    int touchdownFrames = (int)(kSampleRate * kTouchdownSeconds);
    std::vector<short> touchdown(touchdownFrames);
    float touchdownPhase = 0.0f;
    for (int i = 0; i < touchdownFrames; i++) {
        float t = (float)i / (float)kSampleRate;
        float freq = 45.0f + 55.0f * expf(-t * 25.0f);
        touchdownPhase += 2.0f * PI * freq / (float)kSampleRate;
        float value = sinf(touchdownPhase) * expf(-t * 12.0f);
        touchdown[i] = (short)(Clamp(value, -1.0f, 1.0f) * 26000.0f);
    }
    wave.frameCount = touchdownFrames;
    wave.data = touchdown.data();
    audio.touchdownSound = LoadSoundFromWave(wave);

    constexpr float kWindSeconds = 2.0f;
    int windFrames = (int)(kSampleRate * kWindSeconds);
    std::vector<short> wind(windFrames);
    constexpr int kFade = 2000;
    std::vector<float> raw(windFrames + kFade);
    float windLowpass = 0.0f;
    for (int i = 0; i < windFrames + kFade; i++) {
        rng = rng * 1664525u + 1013904223u;
        float noise = ((rng >> 8) & 0xFFFF) / 32768.0f - 1.0f;
        windLowpass += 0.12f * (noise - windLowpass);
        raw[i] = windLowpass;
    }
    // The head blends from the noise that follows the tail, so wind[N-1] -> wind[0] is continuous.
    for (int i = 0; i < windFrames; i++) {
        float v = raw[i];
        if (i < kFade) {
            float k = (float)i / (float)kFade;
            v = raw[i] * k + raw[windFrames + i] * (1.0f - k);
        }
        wind[i] = (short)(Clamp(v * 2.5f, -1.0f, 1.0f) * 20000.0f);
    }
    wave.frameCount = windFrames;
    wave.data = wind.data();
    audio.windSound = LoadSoundFromWave(wave);

    constexpr float kClickSeconds = 0.06f;
    int clickFrames = (int)(kSampleRate * kClickSeconds);
    std::vector<short> click(clickFrames);
    for (int i = 0; i < clickFrames; i++) {
        float t = (float)i / (float)kSampleRate;
        float value = sinf(2.0f * PI * 1200.0f * t) * expf(-t * 70.0f);
        click[i] = (short)(Clamp(value, -1.0f, 1.0f) * 14000.0f);
    }
    wave.frameCount = clickFrames;
    wave.data = click.data();
    audio.clickSound = LoadSoundFromWave(wave);

    constexpr float kDamageSeconds = 0.3f;
    int damageFrames = (int)(kSampleRate * kDamageSeconds);
    std::vector<short> damage(damageFrames);
    float damagePhase = 0.0f;
    float damageLowpass = 0.0f;
    for (int i = 0; i < damageFrames; i++) {
        float t = (float)i / (float)kSampleRate;
        rng = rng * 1664525u + 1013904223u;
        float noise = ((rng >> 8) & 0xFFFF) / 32768.0f - 1.0f;
        damageLowpass += 0.3f * (noise - damageLowpass);
        damagePhase += 2.0f * PI * (70.0f + 90.0f * expf(-t * 30.0f)) / (float)kSampleRate;
        float value = 0.7f * sinf(damagePhase) * expf(-t * 14.0f) + 0.6f * damageLowpass * expf(-t * 25.0f);
        damage[i] = (short)(Clamp(value, -1.0f, 1.0f) * 24000.0f);
    }
    wave.frameCount = damageFrames;
    wave.data = damage.data();
    audio.damageSound = LoadSoundFromWave(wave);

    constexpr float kBeepSeconds = 0.09f;
    int beepFrames = (int)(kSampleRate * kBeepSeconds);
    std::vector<short> beep(beepFrames);
    for (int i = 0; i < beepFrames; i++) {
        float t = (float)i / (float)kSampleRate;
        float env = Clamp(t / 0.004f, 0.0f, 1.0f) * Clamp((kBeepSeconds - t) / 0.01f, 0.0f, 1.0f);
        beep[i] = (short)(sinf(2.0f * PI * 1500.0f * t) * env * 12000.0f);
    }
    wave.frameCount = beepFrames;
    wave.data = beep.data();
    audio.beepSound = LoadSoundFromWave(wave);
}

void StopEngineAudio(EngineAudio &audio) {
    if (IsSoundPlaying(audio.engineSound)) StopSound(audio.engineSound);
}

void PlayCrashSound(EngineAudio &audio, float s) {
    PlaySound(audio.crashSound);
    UnloadSound(audio.boomSound);
    audio.boomSound = Sound{};
    s = Clamp(s, 0.0f, 1.0f);
    if (s < kNoBoomBelow) return;

    float seconds = 0.8f + 1.8f * s;
    float gain = 0.25f + 0.75f * s;
    int frames = (int)(kSampleRate * seconds);
    std::vector<short> boom(frames);
    unsigned int rng = 777u;
    float lowpass = 0.0f;
    float phase = 0.0f;
    float decay = 4.0f / seconds;
    for (int i = 0; i < frames; i++) {
        float t = (float)i / (float)kSampleRate;
        rng = rng * 1664525u + 1013904223u;
        float noise = ((rng >> 8) & 0xFFFF) / 32768.0f - 1.0f;
        lowpass += 0.04f * (noise - lowpass);
        phase += 2.0f * PI * (28.0f + 40.0f * expf(-t * 6.0f)) / (float)kSampleRate;
        float env = expf(-t * decay);
        float attack = fminf(t / 0.01f, 1.0f);
        float value = (0.7f * sinf(phase) + 2.5f * lowpass) * env * attack;
        boom[i] = (short)(Clamp(value, -1.0f, 1.0f) * 30000.0f);
    }
    Wave wave{};
    wave.frameCount = frames;
    wave.sampleRate = kSampleRate;
    wave.sampleSize = 16;
    wave.channels = 1;
    wave.data = boom.data();
    audio.boomSound = LoadSoundFromWave(wave);
    SetSoundVolume(audio.boomSound, gain);
    PlaySound(audio.boomSound);
    TraceLog(LOG_INFO, "AUDIO: crash s=%.2f boom gain=%.2f length=%.2fs", s, gain, seconds);
}

void PlayChimeSound(EngineAudio &audio) {
    PlaySound(audio.chimeSound);
}

void PlayTouchdownSound(EngineAudio &audio) {
    PlaySound(audio.touchdownSound);
}

void PlayClickSound(EngineAudio &audio) {
    PlaySound(audio.clickSound);
}

void PlayDamageSound(EngineAudio &audio) {
    PlaySound(audio.damageSound);
}

void PlayAglBeepSound(EngineAudio &audio) {
    PlaySound(audio.beepSound);
}

float SetMasterVolumeClamped(float volume) {
    volume = Clamp(roundf(volume * 10.0f) / 10.0f, 0.0f, 1.0f);
    SetMasterVolume(volume);
    return volume;
}

void StopWindAudio(EngineAudio &audio) {
    if (IsSoundPlaying(audio.windSound)) StopSound(audio.windSound);
}

void StopOneShotSounds(EngineAudio &audio) {
    StopSound(audio.chimeSound);
    StopSound(audio.touchdownSound);
    StopSound(audio.crashSound);
    StopSound(audio.damageSound);
    StopSound(audio.beepSound);
    if (audio.boomSound.frameCount > 0) StopSound(audio.boomSound);
}

void UpdateWindAudio(EngineAudio &audio, float speedFraction, bool airborne) {
    float t = Clamp(speedFraction, 0.0f, 1.0f);
    float volume = t * t * 0.7f * (airborne ? 1.0f : 0.25f);
    if (volume < 0.01f) {
        StopWindAudio(audio);
        return;
    }
    if (!IsSoundPlaying(audio.windSound)) PlaySound(audio.windSound);
    SetSoundPitch(audio.windSound, 0.6f + t * 0.9f);
    SetSoundVolume(audio.windSound, volume);
}

void UpdateEngineAudio(EngineAudio &audio, float power, float speedFraction) {
    float t = Clamp(power, 0.0f, 1.0f);
    float pitch = 0.7f + t * 0.8f;
    float volume = 0.35f + t * 0.35f;
    if (t <= 0.0f) {
        float windmill = Clamp(speedFraction * 4.0f, 0.0f, 1.0f);
        if (windmill < 0.05f) {
            StopEngineAudio(audio);
            return;
        }
        pitch = 0.5f;
        volume = 0.12f * windmill;
    }

    if (!IsSoundPlaying(audio.engineSound)) {
        PlaySound(audio.engineSound);
    }
    SetSoundPitch(audio.engineSound, pitch);
    SetSoundVolume(audio.engineSound, volume);
}

void UnloadEngineAudio(EngineAudio &audio) {
    UnloadSound(audio.engineSound);
    UnloadSound(audio.crashSound);
    UnloadSound(audio.chimeSound);
    UnloadSound(audio.windSound);
    UnloadSound(audio.touchdownSound);
    UnloadSound(audio.clickSound);
    UnloadSound(audio.damageSound);
    UnloadSound(audio.beepSound);
    UnloadSound(audio.boomSound);
}
