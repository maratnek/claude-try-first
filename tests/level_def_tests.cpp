#include "level_def.h"
#include <cmath>
#include <cstdio>

namespace {

int g_failures = 0;

void Expect(const char *name, bool cond) {
    if (!cond) {
        g_failures++;
        printf("FAIL %s\n", name);
    }
}

bool Near(float a, float b) { return std::fabs(a - b) < 0.0001f; }

bool Matches(const CheckpointDef &c, float x, float y, float z, float r) {
    return Near(c.x, x) && Near(c.y, y) && Near(c.z, z) && Near(c.radius, r);
}

bool Matches(const ObstacleDef &o, float x, float z, float h, float r) {
    return Near(o.x, x) && Near(o.z, z) && Near(o.height, h) && Near(o.radius, r);
}

}  // namespace

int main() {
    const LevelDef &d = Level1Def();

    Expect("gate distance", Near(d.gateDistance, 1000.0f));
    Expect("medal times", Near(d.medalGold, 24.0f) && Near(d.medalSilver, 30.0f) && Near(d.medalBronze, 40.0f));
    Expect("landing rules", d.requireLanding && Near(d.landingZoneLength, 300.0f) &&
                                Near(d.landingZoneWidth, 40.0f) && Near(d.rolloutSpeed, 8.0f));

    Expect("checkpoint count", d.checkpoints.size() == 4);
    if (d.checkpoints.size() == 4) {
        Expect("checkpoint 0", Matches(d.checkpoints[0], 10.0f, 15.0f, 150.0f, 6.0f));
        Expect("checkpoint 1", Matches(d.checkpoints[1], -10.0f, 25.0f, 350.0f, 6.0f));
        Expect("checkpoint 2", Matches(d.checkpoints[2], 8.0f, 12.0f, 550.0f, 6.0f));
        Expect("checkpoint 3", Matches(d.checkpoints[3], -8.0f, 20.0f, 750.0f, 6.0f));
    }

    Expect("hard obstacle count", d.hardObstacles.size() == 4);
    if (d.hardObstacles.size() == 4) {
        Expect("hard obstacle 0", Matches(d.hardObstacles[0], -8.0f, 300.0f, 6.0f, 3.0f));
        Expect("hard obstacle 1", Matches(d.hardObstacles[1], 10.0f, 500.0f, 6.0f, 3.0f));
        Expect("hard obstacle 2", Matches(d.hardObstacles[2], -6.0f, 700.0f, 6.0f, 3.0f));
        Expect("hard obstacle 3", Matches(d.hardObstacles[3], 9.0f, 850.0f, 6.0f, 3.0f));
    }

    Expect("soft obstacle count", d.softObstacles.size() == 4);
    if (d.softObstacles.size() == 4) {
        Expect("soft obstacle 0", Matches(d.softObstacles[0], 9.0f, 120.0f, 0.8f, 1.6f));
        Expect("soft obstacle 1", Matches(d.softObstacles[1], -4.0f, 220.0f, 9.0f, 2.5f));
        Expect("soft obstacle 2", Matches(d.softObstacles[2], 6.0f, 420.0f, 14.0f, 2.5f));
        Expect("soft obstacle 3", Matches(d.softObstacles[3], -5.0f, 620.0f, 10.0f, 2.5f));
    }

    Expect("medals ordered gold < silver < bronze", d.medalGold < d.medalSilver && d.medalSilver < d.medalBronze);

    bool sorted = true, inside = true;
    for (size_t i = 0; i < d.checkpoints.size(); i++) {
        const CheckpointDef &c = d.checkpoints[i];
        if (i > 0 && c.z <= d.checkpoints[i - 1].z) sorted = false;
        if (c.z <= 0.0f || c.z >= d.gateDistance) inside = false;
    }
    Expect("checkpoints sorted by z", sorted);
    Expect("checkpoints inside gate distance", inside);

    bool obstaclesInside = true;
    for (const ObstacleDef &o : d.hardObstacles) {
        if (o.z <= 0.0f || o.z >= d.gateDistance) obstaclesInside = false;
    }
    for (const ObstacleDef &o : d.softObstacles) {
        if (o.z <= 0.0f || o.z >= d.gateDistance) obstaclesInside = false;
    }
    Expect("obstacles inside gate distance", obstaclesInside);

    if (g_failures == 0) printf("All level definition checks passed\n");
    return g_failures == 0 ? 0 : 1;
}
