#include "rain.h"
#include <cmath>
#include "raymath.h"
#include "rlgl.h"

namespace {
const float kHalfWidth = 45.0f;
const float kBelow = 20.0f;
const float kAbove = 40.0f;
const float kFallSpeed = 28.0f;
const float kWindX = 4.0f;
const float kDropLength = 1.6f;
const unsigned char kAlpha = 110;

float Wrap(float value, float center, float size, float below) {
    float lo = center - below;
    return lo + fmodf(fmodf(value - lo, size) + size, size);
}

unsigned int NextRandom(unsigned int &seed) {
    seed = seed * 1664525u + 1013904223u;
    return seed >> 8;
}

float Unit(unsigned int &seed) {
    return (float)(NextRandom(seed) & 0xFFFF) / 65536.0f;
}
}  // namespace

void ClearRain(RainState &rain) {
    rain.seeded = false;
}

void UpdateRain(RainState &rain, Vector3 viewPosition, int dropCount, float dt) {
    if (dropCount <= 0) {
        rain.seeded = false;
        return;
    }
    if (dropCount > kMaxRainDrops) dropCount = kMaxRainDrops;

    if (!rain.seeded) {
        unsigned int seed = 12345u;
        for (int i = 0; i < kMaxRainDrops; i++) {
            rain.positions[i] = {viewPosition.x + (Unit(seed) * 2.0f - 1.0f) * kHalfWidth,
                                 viewPosition.y - kBelow + Unit(seed) * (kBelow + kAbove),
                                 viewPosition.z + (Unit(seed) * 2.0f - 1.0f) * kHalfWidth};
        }
        rain.seeded = true;
    }

    for (int i = 0; i < dropCount; i++) {
        Vector3 &p = rain.positions[i];
        p.x += kWindX * dt;
        p.y -= kFallSpeed * dt;
        p.x = Wrap(p.x, viewPosition.x, 2.0f * kHalfWidth, kHalfWidth);
        p.y = Wrap(p.y, viewPosition.y, kBelow + kAbove, kBelow);
        p.z = Wrap(p.z, viewPosition.z, 2.0f * kHalfWidth, kHalfWidth);
    }
}

void DrawRain(const RainState &rain, int dropCount) {
    if (dropCount <= 0 || !rain.seeded) return;
    if (dropCount > kMaxRainDrops) dropCount = kMaxRainDrops;

    Vector3 trail = Vector3Scale(Vector3Normalize({kWindX, -kFallSpeed, 0.0f}), -kDropLength);

    rlDisableDepthMask();
    rlCheckRenderBatchLimit(2 * dropCount);
    rlBegin(RL_LINES);
    for (int i = 0; i < dropCount; i++) {
        const Vector3 &a = rain.positions[i];
        rlColor4ub(190, 210, 255, kAlpha);
        rlVertex3f(a.x, a.y, a.z);
        rlColor4ub(190, 210, 255, 0);
        rlVertex3f(a.x + trail.x, a.y + trail.y, a.z + trail.z);
    }
    rlEnd();
    rlDrawRenderBatchActive();
    rlEnableDepthMask();
}
