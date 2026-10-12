#pragma once
#include "raylib.h"

constexpr int kMaxDustPuffs = 24;

struct DustPuff {
    Vector3 position;
    Vector3 velocity;
    float age;
    float lifetime;
    float size;
};

struct DustState {
    DustPuff puffs[kMaxDustPuffs];
    bool alive[kMaxDustPuffs] = {};
    int head = 0;
    unsigned seed = 0x0d057u;
};

void SpawnDust(DustState &dust, Vector3 groundPoint, float strength01, int budget);
void UpdateDust(DustState &dust, float dt);
void ClearDust(DustState &dust);
int CountLiveDust(const DustState &dust);
void DrawDust(const DustState &dust, const Camera3D &camera);
