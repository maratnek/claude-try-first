#pragma once
#include "raylib.h"
#include "objects/plane.h"
#include "objects/world.h"

constexpr int kMaxSurfacePoints = 128;
constexpr int kMaxSupportPoints = 26 + kMaxSurfacePoints + 8;

struct DebrisPiece {
    int group;  // break group index, or -1 when the piece is a fixed PlanePart
    PlanePart part;
    Vector3 position;  // centre of mass, which is also the rotation pivot
    Vector3 velocity;
    Vector3 pivot;  // plane-space point that `position` tracks
    Quaternion orientation;
    Vector3 angularVel;  // world space, rad/s
    float mass;
    float radius;
    float age;
    float settleTime;
    Quaternion settleFrom, settleTo;
    int pointCount;
    Vector3 points[kMaxSupportPoints];  // extreme and surface-sampled vertices relative to `pivot`, plane axes
    bool settling;
    bool settled;
};

struct ImpactInfo {
    Vector3 point;
    Vector3 velocity;
    float speed;
    bool ground;
    float impulseBoost = 1.0f;  // explosion adds an outward kick on top of the impact energy
};

constexpr int kMaxFixedDebrisParts = PART_PILOT_HEAD;
constexpr int kMaxDebrisPieces = kMaxBreakGroups > kMaxFixedDebrisParts ? kMaxBreakGroups : kMaxFixedDebrisParts;

struct DebrisState {
    DebrisPiece pieces[kMaxDebrisPieces];
    int count = 0;
};

void SpawnDebris(DebrisState &debris, const PlaneModel &planeModel, const PlaneState &plane, const ImpactInfo &impact, int pieceCount);
void UpdateDebris(DebrisState &debris, const WorldState &world, float dt);
void ClearDebris(DebrisState &debris);
float DebrisLowestClearance(const DebrisPiece &piece, const WorldState &world);
unsigned DebrisDetachedParts(const DebrisState &debris);
unsigned DebrisDetachedGroups(const DebrisState &debris);
void DrawDebris(const DebrisState &debris, const PlaneModel &planeModel);
