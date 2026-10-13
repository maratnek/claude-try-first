#include "level.h"
#include <cstdio>

namespace {

int g_failures = 0;

void Expect(const char *name, bool cond) {
    if (!cond) g_failures++;
    if (!cond) printf("FAIL %s\n", name);
}

LevelState CrossedGate() {
    LevelState level;
    UpdateLevel(level, {0.0f, 0.0f, level.targetDistance + 1.0f}, false, 1.0f / 60.0f);
    return level;
}

}  // namespace

int main() {
    const float rollout = Level1Def().rolloutSpeed;

    LevelState taxi = CrossedGate();
    Expect("taxi gate crossed", taxi.gateCrossed);
    Expect("taxi never flown", !taxi.hasFlown);
    Expect("taxi through gate does not finish", !IsRunFinished(taxi, false, 0.0f));

    LevelState flown;
    UpdateLevel(flown, {0.0f, 20.0f, 100.0f}, true, 1.0f / 60.0f);
    UpdateLevel(flown, {0.0f, 0.0f, flown.targetDistance + 1.0f}, false, 1.0f / 60.0f);
    Expect("flown run latches hasFlown", flown.hasFlown);
    if (Level1Def().requireLanding) {
        Expect("flown run airborne not finished", !IsRunFinished(flown, true, rollout));
        Expect("flown run too fast not finished", !IsRunFinished(flown, false, rollout + 1.0f));
        Expect("flown run landed finishes", IsRunFinished(flown, false, rollout));
    }

    LevelState before;
    Expect("gate not crossed not finished", !IsRunFinished(before, false, 0.0f));

    ResetLevelProgress(flown);
    Expect("reset clears hasFlown", !flown.hasFlown);
    Expect("reset clears stageIndex", flown.stageIndex == 0);

    const float dt = 1.0f / 60.0f;
    const float lastRingZ = Level1Def().checkpoints.back().z;
    LevelState run;
    Expect("level uses level 1 by default", run.def == &Level1Def() && run.stageIndex == 0);
    UpdateLevel(run, {0.0f, 0.0f, 0.0f}, false, dt);
    UpdateLevel(run, {0.0f, 0.0f, 50.0f}, false, dt);
    Expect("rolling on the ground stays in takeoff", run.stageIndex == 0);
    UpdateLevel(run, {0.0f, 10.0f, 80.0f}, true, dt);
    Expect("airborne advances to rings", run.stageIndex == 1);
    UpdateLevel(run, {0.0f, 20.0f, lastRingZ - 1.0f}, true, dt);
    Expect("before the last ring stays in rings", run.stageIndex == 1);
    UpdateLevel(run, {0.0f, 20.0f, lastRingZ}, true, dt);
    Expect("reaching the last ring z advances to gate", run.stageIndex == 2);
    UpdateLevel(run, {0.0f, 20.0f, run.targetDistance - 1.0f}, true, dt);
    Expect("before the gate stays in gate", run.stageIndex == 2);
    UpdateLevel(run, {0.0f, 20.0f, run.targetDistance + 1.0f}, true, dt);
    Expect("gate crossed advances to landing", run.stageIndex == 3);
    UpdateLevel(run, {0.0f, 0.0f, run.targetDistance + 50.0f}, false, dt);
    Expect("landing is the last stage", run.stageIndex == 3);
    Expect("landing stage finishes through IsRunFinished", IsRunFinished(run, false, rollout));

    ResetLevelProgress(run);
    Expect("reset returns to takeoff", run.stageIndex == 0);

    LevelState skipper;
    UpdateLevel(skipper, {0.0f, 20.0f, skipper.targetDistance + 1.0f}, true, dt);
    Expect("several stages complete in one update", skipper.stageIndex == 3);

    LevelDef custom = Level1Def();
    custom.medalGold = 5.0f;
    custom.medalSilver = 10.0f;
    custom.medalBronze = 15.0f;
    Expect("medal gold from def", ComputeMedal(custom, 5.0f) == Medal::Gold);
    Expect("medal silver from def", ComputeMedal(custom, 7.0f) == Medal::Silver);
    Expect("medal bronze from def", ComputeMedal(custom, 15.0f) == Medal::Bronze);
    Expect("medal none from def", ComputeMedal(custom, 15.1f) == Medal::None);
    Expect("level 1 gold limit", ComputeMedal(Level1Def(), Level1Def().medalGold) == Medal::Gold);

    custom.gateDistance = 500.0f;
    custom.checkpoints = {{1.0f, 2.0f, 30.0f, 6.0f}, {-1.0f, 3.0f, 60.0f, 4.0f}};
    LevelState built;
    built.def = &custom;
    built.startPosition = {10.0f, 20.0f, -100.0f};
    built.checkpoints.push_back({{0.0f, 0.0f, 0.0f}, 1.0f, true});
    BuildCheckpoints(built);
    Expect("build replaces checkpoints", built.checkpoints.size() == 2);
    Expect("build offsets by start", built.checkpoints[0].position.x == 11.0f && built.checkpoints[0].position.y == 22.0f && built.checkpoints[0].position.z == -70.0f);
    Expect("build copies radius", built.checkpoints[1].radius == 4.0f && !built.checkpoints[1].passed);
    Expect("build sets target distance", built.targetDistance == 500.0f);
    LevelState again = built;
    BuildCheckpoints(again);
    Expect("rebuild is idempotent", again.checkpoints.size() == 2);

    if (g_failures == 0) printf("LevelTests passed\n");
    return g_failures == 0 ? 0 : 1;
}
