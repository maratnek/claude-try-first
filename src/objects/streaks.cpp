#include "streaks.h"
#include "raymath.h"
#include "rlgl.h"

namespace {
const float kFadeStart = 0.6f;
const float kBehindLimit = 15.0f;
const float kAheadMin = 10.0f;
const float kAheadMax = 50.0f;
const float kRadiusMin = 3.0f;
const float kRadiusMax = 12.0f;
const float kMaxLength = 7.0f;

float RandomRange(float lo, float hi) {
    return lo + (hi - lo) * (float)GetRandomValue(0, 1000) / 1000.0f;
}

Vector3 SpawnStreak(Vector3 cameraPosition, Vector3 direction, float ahead) {
    Vector3 right = Vector3CrossProduct(direction, {0.0f, 1.0f, 0.0f});
    if (Vector3LengthSqr(right) < 1e-4f) right = {1.0f, 0.0f, 0.0f};
    right = Vector3Normalize(right);
    Vector3 up = Vector3CrossProduct(right, direction);
    float angle = RandomRange(0.0f, 2.0f * PI);
    float radius = RandomRange(kRadiusMin, kRadiusMax);
    Vector3 p = Vector3Add(cameraPosition, Vector3Scale(direction, ahead));
    p = Vector3Add(p, Vector3Scale(right, cosf(angle) * radius));
    return Vector3Add(p, Vector3Scale(up, sinf(angle) * radius));
}
}  // namespace

void ClearStreaks(StreakState &streaks) {
    for (int i = 0; i < kMaxSpeedStreaks; i++) streaks.alive[i] = false;
    streaks.intensity = 0.0f;
}

void UpdateStreaks(StreakState &streaks, Vector3 cameraPosition, Vector3 velocityDirection, float speedRatio, bool active, int streakCount) {
    float intensity = (speedRatio - kFadeStart) / (1.0f - kFadeStart);
    if (!active || streakCount <= 0 || intensity <= 0.0f) {
        ClearStreaks(streaks);
        return;
    }
    if (streakCount > kMaxSpeedStreaks) streakCount = kMaxSpeedStreaks;
    streaks.intensity = fminf(intensity, 1.0f);
    streaks.direction = velocityDirection;

    for (int i = 0; i < kMaxSpeedStreaks; i++) {
        if (i >= streakCount) {
            streaks.alive[i] = false;
            continue;
        }
        Vector3 offset = Vector3Subtract(streaks.positions[i], cameraPosition);
        bool passed = Vector3DotProduct(offset, velocityDirection) < -kBehindLimit;
        if (!streaks.alive[i]) {
            streaks.positions[i] = SpawnStreak(cameraPosition, velocityDirection, RandomRange(-kBehindLimit, kAheadMax));
            streaks.alive[i] = true;
        } else if (passed) {
            streaks.positions[i] = SpawnStreak(cameraPosition, velocityDirection, RandomRange(kAheadMin, kAheadMax));
        }
    }
}

void DrawStreaks(const StreakState &streaks) {
    if (streaks.intensity <= 0.0f) return;
    float length = kMaxLength * (0.4f + 0.6f * streaks.intensity);
    unsigned char alpha = (unsigned char)(140.0f * streaks.intensity);

    rlDisableDepthMask();
    for (int i = 0; i < kMaxSpeedStreaks; i++) {
        if (!streaks.alive[i]) continue;
        rlCheckRenderBatchLimit(2);
        rlBegin(RL_LINES);
        Vector3 a = streaks.positions[i];
        Vector3 b = Vector3Add(a, Vector3Scale(streaks.direction, length));
        rlColor4ub(255, 255, 255, 0);
        rlVertex3f(a.x, a.y, a.z);
        rlColor4ub(255, 255, 255, alpha);
        rlVertex3f(b.x, b.y, b.z);
        rlEnd();
    }
    rlDrawRenderBatchActive();
    rlEnableDepthMask();
}
