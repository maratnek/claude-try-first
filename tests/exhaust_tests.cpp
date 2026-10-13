#include "objects/exhaust.h"
#include <cstdio>

namespace {
int g_failures = 0;

void Expect(const char *name, bool cond) {
    if (!cond) g_failures++;
    printf("%s %s\n", cond ? "ok  " : "FAIL", name);
}

int SteadyAlive(float power, bool airborne, int budget) {
    ExhaustState exhaust;
    PlaneState plane;
    plane.enginePower = power;
    plane.airborne = airborne;
    for (int i = 0; i < 120; i++) UpdateExhaust(exhaust, plane, true, budget, 1.0f / 60.0f);
    return ExhaustAliveCount(exhaust);
}
}  // namespace

int main() {
    Expect("rate rises with throttle", ExhaustEmitRate(1.0f, false, 16) > ExhaustEmitRate(0.0f, false, 16));
    Expect("idle still emits", ExhaustEmitRate(0.0f, false, 16) > 0.0f);
    Expect("burst thicker than full throttle", ExhaustEmitRate(1.0f, true, 16) > ExhaustEmitRate(1.0f, false, 16));
    Expect("budget 0 is off", ExhaustEmitRate(1.0f, true, 0) == 0.0f);
    Expect("more puffs at full power than idle", SteadyAlive(1.0f, true, 32) > SteadyAlive(0.0f, true, 32));
    Expect("alive count within budget", SteadyAlive(1.0f, true, 8) <= 8);

    ExhaustState off;
    PlaneState plane;
    plane.enginePower = 1.0f;
    UpdateExhaust(off, plane, true, 0, 0.1f);
    Expect("budget 0 spawns nothing", ExhaustAliveCount(off) == 0);

    ExhaustState stopped;
    UpdateExhaust(stopped, plane, false, 16, 0.5f);
    Expect("not running spawns nothing", ExhaustAliveCount(stopped) == 0);

    ExhaustState start;
    plane.enginePower = 0.0f;
    for (int i = 0; i < 180; i++) UpdateExhaust(start, plane, true, 32, 1.0f / 60.0f);
    int idleAlive = ExhaustAliveCount(start);
    plane.enginePower = 0.05f;
    for (int i = 0; i < 20; i++) UpdateExhaust(start, plane, true, 32, 1.0f / 60.0f);
    Expect("throttle up on ground triggers a burst", ExhaustAliveCount(start) > idleAlive);

    ExhaustState cleared = start;
    ClearExhaust(cleared);
    Expect("clear removes puffs", ExhaustAliveCount(cleared) == 0);

    printf("%d failures\n", g_failures);
    return g_failures == 0 ? 0 : 1;
}
