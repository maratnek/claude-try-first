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

    if (g_failures == 0) printf("LevelTests passed\n");
    return g_failures == 0 ? 0 : 1;
}
