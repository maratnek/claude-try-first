#pragma once
#include <vector>

struct CheckpointDef {
    float x, y, z;
    float radius;
};

struct ObstacleDef {
    float x, z;
    float height;
    float radius;
};

struct HillDef {
    float base;
    float ampX, freqX;
    float ampZ, freqZ;
    float ampXZ, freqXZx, freqXZz;
};

struct TerrainDef {
    int gridSize;
    float worldSize;
    float maxHeight;
    float flatHalfWidth;
    float edgeFade;
    HillDef hill;
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

float HillHeight01(const HillDef &hill, float x, float z);

const LevelDef &Level1Def();
