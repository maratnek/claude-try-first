#include "level_def.h"

namespace {

LevelDef MakeLevel1() {
    LevelDef d;
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
        {-8.0f, 300.0f, 6.0f, 3.0f},
        {10.0f, 500.0f, 6.0f, 3.0f},
        {-6.0f, 700.0f, 6.0f, 3.0f},
        {9.0f, 850.0f, 6.0f, 3.0f},
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
