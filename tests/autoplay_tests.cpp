#include "autoplay.h"
#include <cstdio>
#include <cstring>

namespace {

int g_failures = 0;

void Expect(const char *name, bool cond) {
    if (!cond) {
        g_failures++;
        printf("FAIL %s\n", name);
    }
}

const Vector3 kStart = {0.0f, 2.0f, 0.0f};

void GoodRun(AutoplaySnapshot ends[kStepCount]) {
    for (int i = 0; i < kStepCount; i++) ends[i] = AutoplaySnapshot{};
    ends[kStepMenu].position = kStart;
    for (int i = kStepRotate; i < kStepCount; i++) ends[i].airborne = true;
    ends[kStepRudderD].yaw = -24.0f;
    ends[kStepRudderA].yaw = 0.0f;
    ends[kStepBankLeft].yaw = 45.0f;
    ends[kStepWingsLevel].yaw = 59.0f;
    ends[kStepBankRight].yaw = 14.0f;
    for (int i = kStepCruise; i < kStepRudderD; i++) ends[i].yaw = 0.0f;
}

bool Fails(const AutoplaySnapshot *ends, int count, int logErrors, const char *expectedText) {
    std::vector<std::string> failures = CheckAutoplayResults(ends, count, kStart, logErrors);
    for (const std::string &f : failures) {
        if (f.find(expectedText) != std::string::npos) return true;
    }
    return false;
}

}  // namespace

int main() {
    AutoplaySnapshot ends[kStepCount];

    GoodRun(ends);
    Expect("good run passes", CheckAutoplayResults(ends, kStepCount, kStart, 0).empty());

    GoodRun(ends);
    ends[kStepMenu].position.z = 5.0f;
    Expect("start position off", Fails(ends, kStepCount, 0, "bad start"));

    GoodRun(ends);
    ends[kStepMenu].airborne = true;
    Expect("start airborne", Fails(ends, kStepCount, 0, "bad start"));

    GoodRun(ends);
    ends[kStepMenu].speed = 10.0f;
    Expect("start moving", Fails(ends, kStepCount, 0, "bad start"));

    GoodRun(ends);
    ends[kStepRotate].airborne = false;
    Expect("no liftoff", Fails(ends, kStepCount, 0, "no liftoff"));

    GoodRun(ends);
    ends[kStepRudderD].yaw = -1.0f;
    Expect("dead D rudder", Fails(ends, kStepCount, 0, "rudder D"));

    GoodRun(ends);
    ends[kStepRudderD].yaw = 24.0f;
    Expect("D rudder wrong direction", Fails(ends, kStepCount, 0, "rudder D"));

    GoodRun(ends);
    ends[kStepRudderA].yaw = ends[kStepRudderD].yaw + 1.0f;
    Expect("dead A rudder", Fails(ends, kStepCount, 0, "rudder A"));

    GoodRun(ends);
    ends[kStepRudderA].yaw = ends[kStepRudderD].yaw - 24.0f;
    Expect("A rudder wrong direction", Fails(ends, kStepCount, 0, "rudder A"));

    GoodRun(ends);
    ends[kStepBankLeft].yaw = 2.0f;
    Expect("weak bank left", Fails(ends, kStepCount, 0, "bank left"));

    GoodRun(ends);
    ends[kStepBankRight].yaw = ends[kStepWingsLevel].yaw + 45.0f;
    Expect("bank right wrong direction", Fails(ends, kStepCount, 0, "bank right"));

    GoodRun(ends);
    ends[kStepBankRight].yaw = ends[kStepWingsLevel].yaw - 3.0f;
    Expect("weak bank right", Fails(ends, kStepCount, 0, "bank right"));

    GoodRun(ends);
    ends[kStepCruise].crashed = true;
    Expect("crash in a step", Fails(ends, kStepCruise + 1, 0, "crashed in step cruise"));
    Expect("crash is not also reported as early end", !Fails(ends, kStepCruise + 1, 0, "ended early"));

    GoodRun(ends);
    Expect("script ended early", Fails(ends, kStepCruise + 1, 0, "ended early"));

    GoodRun(ends);
    Expect("logged error", Fails(ends, kStepCount, 2, "logged 2 error"));

    if (g_failures == 0) printf("All autoplay tests passed\n");
    return g_failures == 0 ? 0 : 1;
}
