#pragma once
#include "raylib.h"

constexpr int kMaxSnowFlakes = 200;

struct SnowState {
    Vector3 positions[kMaxSnowFlakes] = {};
    float phases[kMaxSnowFlakes] = {};
    float time = 0.0f;
    bool seeded = false;
};

void ClearSnow(SnowState &snow);
void UpdateSnow(SnowState &snow, Vector3 viewPosition, int flakeCount, float dt);
void DrawSnow(const SnowState &snow, int flakeCount);
