#pragma once
#include "raylib.h"

constexpr int kMaxSmokePuffs = 64;

struct SmokePuff {
    Vector3 position;
    float age;
};

enum class SmokeMode { Trail, ExplosionSmoke };

struct SmokeState {
    SmokePuff puffs[kMaxSmokePuffs];
    int head = 0;
    float spawnTimer = 0.0f;
    bool alive[kMaxSmokePuffs] = {};
    SmokeMode mode = SmokeMode::Trail;
    float lifetime = 1.5f;
    float riseSpeed = 2.0f;
    float sizeScale = 1.0f;
    float darkness = 0.0f;  // 0 grey trail .. 1 black crash column
    Vector3 origin = {0.0f, 0.0f, 0.0f};
    float spread = 0.0f;
    float emitRemaining = 0.0f;
    int puffRate = 0;  // puffs alive at once while emitting
    unsigned seed = 0;
};

void UpdateSmoke(SmokeState &smoke, Vector3 planePosition, Vector3 planeForward, bool emitting, int puffCount, float dt);
void ClearSmoke(SmokeState &smoke);
void DrawSmoke(const SmokeState &smoke, const Camera3D &camera);

// Crash column from the fuel tank: s in [0,1] is the explosion scale. Below 0.05 it is a brief dust burst.
// puffBudget caps the puffs alive at once (A1 preset). Switches the state to ExplosionSmoke until ClearSmoke.
void StartExplosionSmoke(SmokeState &smoke, Vector3 origin, float s, int puffBudget);
void UpdateExplosionSmoke(SmokeState &smoke, float dt);
