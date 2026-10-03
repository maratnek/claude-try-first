#pragma once
#include "raylib.h"

constexpr int kMaxRainDrops = 200;

struct RainState {
    Vector3 positions[kMaxRainDrops] = {};
    bool seeded = false;
};

void ClearRain(RainState &rain);
void UpdateRain(RainState &rain, Vector3 viewPosition, int dropCount, float dt);
void DrawRain(const RainState &rain, int dropCount);
