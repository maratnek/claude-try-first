#include "level_def.h"
#include <cmath>

namespace {

LevelDef MakeLevel1() {
    LevelDef d;
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
    return d;
}

}  // namespace

const LevelDef &Level1Def() {
    static const LevelDef def = MakeLevel1();
    return def;
}

bool CrossesRingHole(float ax, float ay, float az, float bx, float by, float bz, float cx, float cy, float cz, float holeRadius) {
    if ((az - cz) * (bz - cz) > 0.0f || az == bz) return false;
    float t = (cz - az) / (bz - az);
    float hx = ax + (bx - ax) * t - cx;
    float hy = ay + (by - ay) * t - cy;
    return std::sqrt(hx * hx + hy * hy) < holeRadius;
}
