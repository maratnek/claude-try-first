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

struct LevelDef {
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
