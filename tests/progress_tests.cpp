#include "progress.h"
#include <cstdio>
#include <cmath>

namespace {

int g_failures = 0;

void Expect(const char *name, bool cond) {
    if (!cond) {
        g_failures++;
        printf("FAIL %s\n", name);
    }
}

bool Near(float a, float b) { return std::fabs(a - b) < 0.001f; }

}  // namespace

int main() {
    ProgressData empty = ParseProgress("");
    Expect("empty text gives defaults", empty.levels.empty() && empty.unlocks.empty() && Near(empty.volume, 1.0f) && empty.preset == -1);

    ProgressData d;
    d.volume = 0.4f;
    d.preset = 1;
    Expect("first result improves", RecordLevelResult(d, "level1", 23.5f, 2));
    Expect("slower result does not improve", !RecordLevelResult(d, "level1", 30.0f, 1));
    Expect("slower result keeps time", Near(d.levels[0].bestTime, 23.5f));
    Expect("lower stars keep higher stars", d.levels[0].stars == 2);
    Expect("faster result improves", RecordLevelResult(d, "level1", 21.0f, 3));
    Expect("stars capped at 3", (RecordLevelResult(d, "level1", 21.0f, 9), d.levels[0].stars == 3));
    Expect("invalid id rejected", !RecordLevelResult(d, "bad id=1", 10.0f, 1) && d.levels.size() == 1);
    RecordLevelResult(d, "level2", -1.0f, 1);
    AddUnlock(d, "biplane_red");
    AddUnlock(d, "biplane_red");
    Expect("unlock added once", d.unlocks.size() == 1 && HasUnlock(d, "biplane_red") && !HasUnlock(d, "other"));

    ProgressData back = ParseProgress(SerializeProgress(d));
    Expect("round trip volume", Near(back.volume, 0.4f));
    Expect("round trip preset", back.preset == 1);
    Expect("round trip levels", back.levels.size() == 2 && back.levels[0].id == "level1" && Near(back.levels[0].bestTime, 21.0f) && back.levels[0].stars == 3);
    Expect("round trip unset time", back.levels[1].id == "level2" && back.levels[1].bestTime < 0.0f && back.levels[1].stars == 1);
    Expect("round trip unlocks", back.unlocks.size() == 1 && back.unlocks[0] == "biplane_red");
    Expect("serialization is stable", SerializeProgress(back) == SerializeProgress(d));

    ProgressData future = ParseProgress("version=2\nvolume=0.2\nlevel.level1=5.000,3\n");
    Expect("unknown version gives defaults", future.levels.empty() && Near(future.volume, 1.0f));
    ProgressData unversioned = ParseProgress("volume=0.2\n");
    Expect("unversioned text ignored", Near(unversioned.volume, 1.0f));

    ProgressData messy = ParseProgress(
        "version=1\r\nvolume=7\r\npreset=9\r\nlevel.a=12.5,2\r\nlevel.a=1.0,3\r\nlevel.b=oops\r\nlevel.=3,1\r\n"
        "level.c=-4.0,99\r\nunlock.x=1\r\nunlock.y=0\r\ngarbage line\r\nnew_key=5\r\n");
    Expect("volume clamped", Near(messy.volume, 1.0f));
    Expect("bad preset ignored", messy.preset == -1);
    Expect("duplicate level keeps first", messy.levels.size() == 2 && messy.levels[0].id == "a" && Near(messy.levels[0].bestTime, 12.5f) && messy.levels[0].stars == 2);
    Expect("bad time and stars clamped", messy.levels[1].id == "c" && messy.levels[1].bestTime < 0.0f && messy.levels[1].stars == 3);
    Expect("only enabled unlocks kept", messy.unlocks.size() == 1 && messy.unlocks[0] == "x");

    if (g_failures == 0) printf("ProgressTests: all passed\n");
    return g_failures == 0 ? 0 : 1;
}
