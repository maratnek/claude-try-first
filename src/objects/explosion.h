#pragma once
#include "raylib.h"

struct ExplosionState {
    bool active = false;
    float age = 0.0f;
    float scale = 0.0f;  // fuel-driven s in [0,1]
    Vector3 centre = {0.0f, 0.0f, 0.0f};
    int fireballs = 0;   // 0 draws the orange ring instead
    bool flash = false;
};

// s in [0,1] is the fuel fraction. Below 0.05 nothing is shown (breakup only).
// fireballs: 0 = orange ring sprite only (Low), 1..3 = fireball spheres.
void StartExplosion(ExplosionState &explosion, Vector3 centre, float s, int fireballs, bool flash);
void UpdateExplosion(ExplosionState &explosion, float dt);
void ClearExplosion(ExplosionState &explosion);
void DrawExplosion(const ExplosionState &explosion, const Camera3D &camera);
void DrawExplosionFlash(const ExplosionState &explosion);
