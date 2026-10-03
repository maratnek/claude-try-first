#pragma once
#include "raylib.h"

constexpr int kMaxSpeedStreaks = 40;

struct StreakState {
    Vector3 positions[kMaxSpeedStreaks];
    bool alive[kMaxSpeedStreaks] = {};
    Vector3 direction = {0.0f, 0.0f, 1.0f};
    float intensity = 0.0f;
};

void ClearStreaks(StreakState &streaks);
void UpdateStreaks(StreakState &streaks, Vector3 cameraPosition, Vector3 velocityDirection, float speedRatio, bool active, int streakCount);
void DrawStreaks(const StreakState &streaks);
