#include "objects/dust.h"
#include <cstdio>

namespace {

int g_failures = 0;

void Expect(const char *name, bool cond) {
    if (!cond) g_failures++;
    printf("%s %s\n", cond ? "ok  " : "FAIL", name);
}

const Vector3 kGround = {0.0f, 0.0f, 0.0f};

}  // namespace

int main() {
    DustState dust;

    SpawnDust(dust, kGround, 1.0f, 0);
    Expect("budget 0 spawns nothing", CountLiveDust(dust) == 0);

    SpawnDust(dust, kGround, 1.0f, 24);
    int strong = CountLiveDust(dust);
    ClearDust(dust);
    SpawnDust(dust, kGround, 0.25f, 24);
    int weak = CountLiveDust(dust);
    ClearDust(dust);
    Expect("stronger impact spawns more puffs", strong > weak && weak > 0);
    Expect("full strength fills the pool", strong == kMaxDustPuffs);

    SpawnDust(dust, kGround, 1.0f, 100);
    Expect("budget above pool is capped", CountLiveDust(dust) == kMaxDustPuffs);
    ClearDust(dust);

    SpawnDust(dust, kGround, 0.5f, 10);
    Expect("puffs start at ground level", dust.puffs[0].position.y < 0.5f);
    for (int i = 0; i < 30; i++) UpdateDust(dust, 1.0f / 60.0f);
    Expect("puffs alive mid-life", CountLiveDust(dust) > 0);
    Expect("puffs rise", dust.puffs[0].position.y > 0.2f);
    for (int i = 0; i < 120; i++) UpdateDust(dust, 1.0f / 60.0f);
    Expect("puffs die after lifetime", CountLiveDust(dust) == 0);

    SpawnDust(dust, kGround, 1.0f, 24);
    ClearDust(dust);
    Expect("clear empties the pool", CountLiveDust(dust) == 0);

    printf("%s\n", g_failures ? "FAILED" : "all passed");
    return g_failures ? 1 : 0;
}
