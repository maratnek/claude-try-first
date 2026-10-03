#pragma once
#include "raylib.h"

constexpr int kMaxSmokePuffs = 64;

struct SmokePuff {
    Vector3 position;
    float age;
};

struct SmokeState {
    SmokePuff puffs[kMaxSmokePuffs];
    int head = 0;
    float spawnTimer = 0.0f;
    bool alive[kMaxSmokePuffs] = {};
};

void UpdateSmoke(SmokeState &smoke, Vector3 planePosition, Vector3 planeForward, bool emitting, int puffCount, float dt);
void ClearSmoke(SmokeState &smoke);
void DrawSmoke(const SmokeState &smoke, const Camera3D &camera);
