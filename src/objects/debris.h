#pragma once
#include "raylib.h"
#include "objects/plane.h"
#include "objects/world.h"

struct DebrisPiece {
    int group;  // break group index, or -1 when the piece is a fixed PlanePart
    PlanePart part;
    Vector3 position;
    Vector3 velocity;
    Vector3 pivot;
    Vector3 spinAngle;
    Vector3 spinRate;
    float yaw, pitch, roll;
    float restHeight;
    bool settled;
};

constexpr int kMaxFixedDebrisParts = PART_PILOT_HEAD;
constexpr int kMaxDebrisPieces = kMaxBreakGroups > kMaxFixedDebrisParts ? kMaxBreakGroups : kMaxFixedDebrisParts;

struct DebrisState {
    DebrisPiece pieces[kMaxDebrisPieces];
    int count = 0;
};

void SpawnDebris(DebrisState &debris, const PlaneModel &planeModel, const PlaneState &plane, Vector3 planeVelocity, int pieceCount);
void UpdateDebris(DebrisState &debris, const WorldState &world, int pieceCount, float dt);
void ClearDebris(DebrisState &debris);
unsigned DebrisDetachedParts(const DebrisState &debris);
unsigned DebrisDetachedGroups(const DebrisState &debris);
void DrawDebris(const DebrisState &debris, const PlaneModel &planeModel);
