#pragma once
#include <string>
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

enum class StageType { Takeoff, Rings, Gate, RunwayLanding };

// endZ is relative to the start (like CheckpointDef::z); only Rings and Gate use it.
struct StageDef {
    StageType type;
    const char *title;
    float endZ;
};

struct LevelDef {
    const char *id;
    const char *name;
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
    std::vector<StageDef> stages;
};

const LevelDef &Level1Def();

const std::vector<const LevelDef *> &CampaignLevels();

// Null when no campaign level has this id.
const LevelDef *FindLevel(const std::string &id);

// True when the segment from (ax,ay,az) to (bx,by,bz) crosses the ring plane z = cz
// at a point closer than holeRadius to the ring centre (cx,cy).
bool CrossesRingHole(float ax, float ay, float az, float bx, float by, float bz, float cx, float cy, float cz, float holeRadius);
