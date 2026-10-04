#include "snow.h"
#include <cmath>
#include "rlgl.h"

namespace {
const float kHalfWidth = 40.0f;
const float kBelow = 15.0f;
const float kAbove = 30.0f;
const float kFallSpeed = 2.5f;
const float kDriftSpeed = 1.2f;
const float kFlakeSize = 0.25f;

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

void ClearSnow(SnowState &snow) {
    snow.seeded = false;
}

void UpdateSnow(SnowState &snow, Vector3 viewPosition, int flakeCount, float dt) {
    if (flakeCount <= 0) {
        snow.seeded = false;
        return;
    }
    if (flakeCount > kMaxSnowFlakes) flakeCount = kMaxSnowFlakes;

    if (!snow.seeded) {
        unsigned int seed = 67890u;
        for (int i = 0; i < kMaxSnowFlakes; i++) {
            snow.positions[i] = {viewPosition.x + (Unit(seed) * 2.0f - 1.0f) * kHalfWidth,
                                 viewPosition.y - kBelow + Unit(seed) * (kBelow + kAbove),
                                 viewPosition.z + (Unit(seed) * 2.0f - 1.0f) * kHalfWidth};
            snow.phases[i] = Unit(seed) * 6.2831853f;
        }
        snow.time = 0.0f;
        snow.seeded = true;
    }

    snow.time += dt;
    for (int i = 0; i < flakeCount; i++) {
        Vector3 &p = snow.positions[i];
        p.x += sinf(snow.time * 0.7f + snow.phases[i]) * kDriftSpeed * dt;
        p.z += cosf(snow.time * 0.5f + snow.phases[i] * 1.7f) * kDriftSpeed * dt;
        p.y -= kFallSpeed * (0.7f + 0.3f * sinf(snow.phases[i] * 3.0f)) * dt;
        p.x = Wrap(p.x, viewPosition.x, 2.0f * kHalfWidth, kHalfWidth);
        p.y = Wrap(p.y, viewPosition.y, kBelow + kAbove, kBelow);
        p.z = Wrap(p.z, viewPosition.z, 2.0f * kHalfWidth, kHalfWidth);
    }
}

void DrawSnow(const SnowState &snow, int flakeCount) {
    if (flakeCount <= 0 || !snow.seeded) return;
    if (flakeCount > kMaxSnowFlakes) flakeCount = kMaxSnowFlakes;

    rlDrawRenderBatchActive();
    rlDisableDepthMask();
    rlCheckRenderBatchLimit(2 * flakeCount);
    rlBegin(RL_LINES);
    for (int i = 0; i < flakeCount; i++) {
        const Vector3 &a = snow.positions[i];
        rlColor4ub(255, 255, 255, 255);
        rlVertex3f(a.x, a.y, a.z);
        rlVertex3f(a.x + kFlakeSize, a.y + kFlakeSize, a.z);
    }
    rlEnd();
    rlDrawRenderBatchActive();
    rlEnableDepthMask();
}
