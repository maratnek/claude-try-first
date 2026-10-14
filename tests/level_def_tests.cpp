#include "level_def.h"
#include <cmath>
#include <cstdio>
#include <string>

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
    Expect("ring: straight through the centre counts", CrossesRingHole(0, 15, 140, 0, 15, 160, 0, 15, 150, 5.0f));
    Expect("ring: 4 m off centre counts", CrossesRingHole(4, 15, 149, 4, 15, 151, 0, 15, 150, 5.0f));
    Expect("ring: clipping the rim does not count", !CrossesRingHole(5.5f, 15, 149, 5.5f, 15, 151, 0, 15, 150, 5.0f));
    Expect("ring: passing beside it does not count", !CrossesRingHole(9, 15, 140, 9, 15, 160, 0, 15, 150, 5.0f));
    Expect("ring: not reached yet", !CrossesRingHole(0, 15, 140, 0, 15, 149, 0, 15, 150, 5.0f));
    Expect("ring: diagonal crossing through the hole", CrossesRingHole(-3, 12, 148, 3, 18, 152, 0, 15, 150, 5.0f));
    const LevelDef &d = Level1Def();

    Expect("start position", Near(d.startX, 0.0f) && Near(d.startZ, 0.0f) && Near(d.startHeightAboveGround, 0.3f));
    Expect("terrain shape", d.terrain.gridSize == 64 && Near(d.terrain.worldSize, 1400.0f) &&
                                Near(d.terrain.maxHeight, 10.0f) && Near(d.terrain.flatHalfWidth, 40.0f) &&
                                Near(d.terrain.edgeFade, 0.12f));
    Expect("flat corridor covers checkpoints and obstacles", [&] {
        for (const ObstacleDef &o : d.hardObstacles) if (std::fabs(o.x) >= d.terrain.flatHalfWidth) return false;
        for (const ObstacleDef &o : d.softObstacles) if (std::fabs(o.x) >= d.terrain.flatHalfWidth) return false;
        for (const CheckpointDef &c : d.checkpoints) if (std::fabs(c.x) >= d.terrain.flatHalfWidth) return false;
        return true;
    }());
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
        Expect("hard obstacle 0", Matches(d.hardObstacles[0], -8.0f, 300.0f, 16.0f, 5.0f));
        Expect("hard obstacle 1", Matches(d.hardObstacles[1], 10.0f, 500.0f, 16.0f, 5.0f));
        Expect("hard obstacle 2", Matches(d.hardObstacles[2], -6.0f, 700.0f, 16.0f, 5.0f));
        Expect("hard obstacle 3", Matches(d.hardObstacles[3], 9.0f, 850.0f, 16.0f, 5.0f));
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

    Expect("level identity", std::string(d.id) == "level1" && std::string(d.name) == "Valley Run");
    Expect("stage count", d.stages.size() == 4);
    if (d.stages.size() == 4) {
        Expect("stage types", d.stages[0].type == StageType::Takeoff && d.stages[1].type == StageType::Rings &&
                                  d.stages[2].type == StageType::Gate && d.stages[3].type == StageType::RunwayLanding);
        Expect("stage titles", std::string(d.stages[0].title) == "Takeoff" && std::string(d.stages[1].title) == "Rings" &&
                                   std::string(d.stages[2].title) == "Gate" && std::string(d.stages[3].title) == "Landing");
        Expect("rings stage ends at the last checkpoint", Near(d.stages[1].endZ, d.checkpoints.back().z));
        Expect("gate stage ends at the gate distance", Near(d.stages[2].endZ, d.gateDistance));
    }

    const LevelDef &t = TutTakeoffDef();
    const LevelDef &u = TutTurnsDef();
    Expect("registry holds both tutorials then level 1", CampaignLevels().size() == 3 && CampaignLevels()[0] == &t && CampaignLevels()[1] == &u && CampaignLevels()[2] == &d);
    Expect("campaign ids unique", [&] {
        for (size_t i = 0; i < CampaignLevels().size(); i++)
            for (size_t j = i + 1; j < CampaignLevels().size(); j++)
                if (std::string(CampaignLevels()[i]->id) == CampaignLevels()[j]->id) return false;
        return true;
    }());
    Expect("FindLevel tut_takeoff", FindLevel("tut_takeoff") == &t && std::string(t.name) == "First Wings");
    Expect("tut: landing rules", t.requireLanding && Near(t.gateDistance, 700.0f) && t.landingZoneLength > 0.0f);
    Expect("tut: no obstacles", t.hardObstacles.empty() && t.softObstacles.empty());
    Expect("tut: terrain nearly flat", t.terrain.maxHeight <= 1.0f);
    Expect("tut: medals ordered", t.medalGold < t.medalSilver && t.medalSilver < t.medalBronze);
    Expect("tut: seven rings", t.checkpoints.size() == 7);
    if (t.checkpoints.size() == 7) {
        Expect("tut: climb rings", Matches(t.checkpoints[0], 0.0f, 25.0f, 100.0f, 14.0f) &&
                                       Matches(t.checkpoints[1], 0.0f, 50.0f, 250.0f, 14.0f));
        bool ordered = true, inCorridor = true, spaced = true, descending = true;
        for (size_t i = 0; i < t.checkpoints.size(); i++) {
            const CheckpointDef &c = t.checkpoints[i];
            if (i > 0 && c.z <= t.checkpoints[i - 1].z) ordered = false;
            if (std::fabs(c.x) + c.radius >= t.terrain.flatHalfWidth) inCorridor = false;
            if (i >= 3 && !Near(c.z - t.checkpoints[i - 1].z, 80.0f)) spaced = false;
            if (i >= 2 && (c.radius != 12.0f || (i > 2 && c.y >= t.checkpoints[i - 1].y))) descending = false;
        }
        Expect("tut: rings sorted by z", ordered);
        Expect("tut: rings inside flat corridor", inCorridor);
        Expect("tut: approach rings 80 m apart", spaced);
        Expect("tut: glide-path ring heights decrease", descending);
        bool glide = true;
        for (size_t i = 2; i < 7; i++) {
            float deg = std::atan2(t.checkpoints[i].y, t.gateDistance - t.checkpoints[i].z) * 57.29578f;
            if (std::fabs(deg - 4.0f) > 0.1f) glide = false;
        }
        Expect("tut: approach rings follow a 4 degree glide path", glide);
        Expect("tut: landing zone beyond last ring", t.gateDistance > t.checkpoints.back().z + t.checkpoints.back().radius);
    }
    Expect("tut: stages", t.stages.size() == 4 && t.stages[0].type == StageType::Takeoff && t.stages[1].type == StageType::Rings &&
                              t.stages[2].type == StageType::Rings && t.stages[3].type == StageType::RunwayLanding &&
                              Near(t.stages[1].endZ, 250.0f) && Near(t.stages[2].endZ, t.checkpoints.back().z));
    Expect("FindLevel tut_turns", FindLevel("tut_turns") == &u && std::string(u.name) == "Slalom Field");
    Expect("slalom: no landing required", !u.requireLanding);
    Expect("slalom: no obstacles", u.hardObstacles.empty() && u.softObstacles.empty());
    Expect("slalom: terrain nearly flat", u.terrain.maxHeight <= 1.0f);
    Expect("slalom: medals ordered", u.medalGold < u.medalSilver && u.medalSilver < u.medalBronze);
    Expect("slalom: eleven rings", u.checkpoints.size() == 11);
    if (u.checkpoints.size() == 11) {
        bool ordered = true, inCorridor = true, alternates = true, spaced = true;
        for (size_t i = 0; i < u.checkpoints.size(); i++) {
            const CheckpointDef &c = u.checkpoints[i];
            if (i > 0 && c.z <= u.checkpoints[i - 1].z) ordered = false;
            if (std::fabs(c.x) + c.radius >= u.terrain.flatHalfWidth) inCorridor = false;
            if (i > 0 && i < 8 && c.x * u.checkpoints[i - 1].x >= 0.0f) alternates = false;
            if (i > 0 && !Near(c.z - u.checkpoints[i - 1].z, 180.0f)) spaced = false;
        }
        Expect("slalom: rings sorted by z", ordered);
        Expect("slalom: rings inside flat corridor", inCorridor);
        Expect("slalom: turn rings alternate sides", alternates);
        Expect("slalom: rings 180 m apart", spaced);
        Expect("slalom: arrow rings are wider than A/D rings", std::fabs(u.checkpoints[0].x) > std::fabs(u.checkpoints[4].x));
        Expect("slalom: throttle rings on the centreline", u.checkpoints[8].x == 0.0f && u.checkpoints[9].x == 0.0f && u.checkpoints[10].x == 0.0f);
        Expect("slalom: gate beyond last ring", u.gateDistance > u.checkpoints.back().z + u.checkpoints.back().radius);
    }
    Expect("slalom: stages", u.stages.size() == 5 && u.stages[0].type == StageType::Takeoff && u.stages[1].type == StageType::Rings &&
                                 u.stages[2].type == StageType::Rings && u.stages[3].type == StageType::Rings && u.stages[4].type == StageType::Gate &&
                                 Near(u.stages[1].endZ, 690.0f) && Near(u.stages[2].endZ, 1410.0f) && Near(u.stages[3].endZ, 1950.0f) &&
                                 Near(u.stages[4].endZ, u.gateDistance));
    Expect("FindLevel level1", FindLevel("level1") == &d);
    Expect("FindLevel unknown id", FindLevel("level9") == nullptr);

    if (g_failures == 0) printf("All level definition checks passed\n");
    return g_failures == 0 ? 0 : 1;
}
