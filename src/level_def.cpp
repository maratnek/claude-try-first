#include "level_def.h"
#include <cmath>

namespace {

LevelDef MakeLevel1() {
    LevelDef d;
    d.id = "level1";
    d.name = "Valley Run";
    d.startX = 0.0f;
    d.startZ = 0.0f;
    d.startHeightAboveGround = 0.3f;
    d.terrain = {64, 1400.0f, 10.0f, 40.0f, 0.12f};
    d.gateDistance = 1000.0f;
    d.medalGold = 24.0f;
    d.medalSilver = 30.0f;
    d.medalBronze = 40.0f;
    d.requireLanding = true;
    d.landingZoneLength = 300.0f;
    d.landingZoneWidth = 40.0f;
    d.rolloutSpeed = 8.0f;
    d.checkpoints = {
        {10.0f, 15.0f, 150.0f, 6.0f},
        {-10.0f, 25.0f, 350.0f, 6.0f},
        {8.0f, 12.0f, 550.0f, 6.0f},
        {-8.0f, 20.0f, 750.0f, 6.0f},
    };
    d.hardObstacles = {
        {-8.0f, 300.0f, 16.0f, 5.0f},
        {10.0f, 500.0f, 16.0f, 5.0f},
        {-6.0f, 700.0f, 16.0f, 5.0f},
        {9.0f, 850.0f, 16.0f, 5.0f},
    };
    d.softObstacles = {
        {9.0f, 120.0f, 0.8f, 1.6f},
        {-4.0f, 220.0f, 9.0f, 2.5f},
        {6.0f, 420.0f, 14.0f, 2.5f},
        {-5.0f, 620.0f, 10.0f, 2.5f},
    };
    d.stages = {
        {StageType::Takeoff, "Takeoff", 0.0f},
        {StageType::Rings, "Rings", d.checkpoints.back().z},
        {StageType::Gate, "Gate", d.gateDistance},
    };
    if (d.requireLanding) d.stages.push_back({StageType::RunwayLanding, "Landing", 0.0f});
    return d;
}

// Approach rings sit on a ~4 degree glide path to the runway threshold at gateDistance.
LevelDef MakeTutTakeoff() {
    LevelDef d;
    d.id = "tut_takeoff";
    d.name = "First Wings";
    d.startX = 0.0f;
    d.startZ = 0.0f;
    d.startHeightAboveGround = 0.3f;
    // maxHeight 0 would divide by zero in the heightmap normalisation; 1 m over a wide flat corridor is flat enough.
    d.terrain = {64, 1400.0f, 1.0f, 100.0f, 0.12f};
    d.gateDistance = 700.0f;
    d.medalGold = 20.0f;
    d.medalSilver = 26.0f;
    d.medalBronze = 36.0f;
    d.requireLanding = true;
    d.landingZoneLength = 300.0f;
    d.landingZoneWidth = 40.0f;
    d.rolloutSpeed = 8.0f;
    d.checkpoints = {
        {0.0f, 25.0f, 100.0f, 14.0f},
        {0.0f, 50.0f, 250.0f, 14.0f},
        {0.0f, 28.0f, 300.0f, 12.0f},
        {0.0f, 22.4f, 380.0f, 12.0f},
        {0.0f, 16.8f, 460.0f, 12.0f},
        {0.0f, 11.2f, 540.0f, 12.0f},
        {0.0f, 5.6f, 620.0f, 12.0f},
    };
    d.stages = {
        {StageType::Takeoff, "Takeoff", 0.0f},
        {StageType::Rings, "Climb", 250.0f},
        {StageType::Rings, "Approach", d.checkpoints.back().z},
        {StageType::RunwayLanding, "Landing", 0.0f},
    };
    return d;
}

LevelDef MakeTutTurns() {
    LevelDef d;
    d.id = "tut_turns";
    d.name = "Slalom Field";
    d.startX = 0.0f;
    d.startZ = 0.0f;
    d.startHeightAboveGround = 0.3f;
    d.terrain = {64, 1400.0f, 1.0f, 100.0f, 0.12f};
    d.gateDistance = 2100.0f;
    d.medalGold = 85.0f;
    d.medalSilver = 105.0f;
    d.medalBronze = 130.0f;
    d.requireLanding = false;
    d.landingZoneLength = 300.0f;
    d.landingZoneWidth = 40.0f;
    d.rolloutSpeed = 8.0f;
    d.checkpoints = {
        {-45.0f, 40.0f, 150.0f, 14.0f},
        {45.0f, 40.0f, 330.0f, 14.0f},
        {-45.0f, 40.0f, 510.0f, 14.0f},
        {45.0f, 40.0f, 690.0f, 14.0f},
        {-30.0f, 40.0f, 870.0f, 14.0f},
        {30.0f, 40.0f, 1050.0f, 14.0f},
        {-30.0f, 40.0f, 1230.0f, 14.0f},
        {30.0f, 40.0f, 1410.0f, 14.0f},
        {0.0f, 40.0f, 1590.0f, 14.0f},
        {0.0f, 30.0f, 1770.0f, 14.0f},
        {0.0f, 40.0f, 1950.0f, 14.0f},
    };
    d.stages = {
        {StageType::Takeoff, "Takeoff", 0.0f},
        {StageType::Rings, "Bank with the arrows", 690.0f},
        {StageType::Rings, "A and D swing the nose", 1410.0f},
        {StageType::Rings, "Throttle: W and S", d.checkpoints.back().z},
        {StageType::Gate, "Gate", d.gateDistance},
    };
    return d;
}

}  // namespace

const LevelDef &TutTurnsDef() {
    static const LevelDef def = MakeTutTurns();
    return def;
}

const LevelDef &TutTakeoffDef() {
    static const LevelDef def = MakeTutTakeoff();
    return def;
}

const LevelDef &Level1Def() {
    static const LevelDef def = MakeLevel1();
    return def;
}

const std::vector<const LevelDef *> &CampaignLevels() {
    static const std::vector<const LevelDef *> levels = {&TutTakeoffDef(), &TutTurnsDef(), &Level1Def()};
    return levels;
}

const LevelDef *FindLevel(const std::string &id) {
    for (const LevelDef *def : CampaignLevels()) {
        if (id == def->id) return def;
    }
    return nullptr;
}

bool CrossesRingHole(float ax, float ay, float az, float bx, float by, float bz, float cx, float cy, float cz, float holeRadius) {
    if ((az - cz) * (bz - cz) > 0.0f || az == bz) return false;
    float t = (cz - az) / (bz - az);
    float hx = ax + (bx - ax) * t - cx;
    float hy = ay + (by - ay) * t - cy;
    return std::sqrt(hx * hx + hy * hy) < holeRadius;
}
