#pragma once
#include <vector>

struct CheckpointDef {
    float x, y, z;
    float radius;
};

// Hard obstacles are pines: height = total height, radius = crown radius (about 0.32 x height to match the drawn tree).
// Soft obstacles are canopies: height = centre above the ground, radius = canopy radius.
struct ObstacleDef {
    float x, z;
    float height;
    float radius;
};

struct TerrainDef {
    int gridSize;
    float worldSize;
    float maxHeight;
    float flatHalfWidth;
    float edgeFade;
};

struct LevelDef {
    float startX, startZ;
    float startHeightAboveGround;
    TerrainDef terrain;
    float gateDistance;
    float medalGold, medalSilver, medalBronze;
    bool requireLanding;
    float landingZoneLength;
    float landingZoneWidth;
    float rolloutSpeed;
    std::vector<CheckpointDef> checkpoints;
    std::vector<ObstacleDef> hardObstacles;
    std::vector<ObstacleDef> softObstacles;
};

const LevelDef &Level1Def();
