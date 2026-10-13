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

// True when the segment from (ax,ay,az) to (bx,by,bz) crosses the ring plane z = cz
// at a point closer than holeRadius to the ring centre (cx,cy).
bool CrossesRingHole(float ax, float ay, float az, float bx, float by, float bz, float cx, float cy, float cz, float holeRadius);
