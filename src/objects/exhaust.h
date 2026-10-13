#pragma once
#include "raylib.h"
#include "flight.h"

constexpr int kMaxExhaustPuffs = 32;

struct ExhaustPuff {
    Vector3 position;
    Vector3 velocity;
    float age;
    float power;
    float burst;
};

struct ExhaustState {
    ExhaustPuff puffs[kMaxExhaustPuffs];
    bool alive[kMaxExhaustPuffs] = {};
    int head = 0;
    int nextPipe = 0;
    float spawnAccum = 0.0f;
    float burstRemaining = 0.0f;
    float prevPower = 0.0f;
    bool started = false;
    unsigned seed = 0x7e3a1u;
};

float ExhaustEmitRate(float enginePower, bool burst, int puffBudget);

void UpdateExhaust(ExhaustState &exhaust, const PlaneState &plane, bool engineRunning, int puffBudget, float dt);
void ClearExhaust(ExhaustState &exhaust);
int ExhaustAliveCount(const ExhaustState &exhaust);
void DrawExhaust(const ExhaustState &exhaust, const Camera3D &camera);
