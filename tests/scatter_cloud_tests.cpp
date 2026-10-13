#include "level_def.h"
#include "objects/world.h"
#include <cmath>
#include <cstdint>
#include <cstdio>

// Stand-in for world.cpp so the generators can be tested without a window; arithmetic only, so it is identical on every platform.
float GetGroundHeight(const WorldState &, float worldX, float worldZ) {
    return 5.0f + 0.004f * worldX - 0.002f * worldZ;
}

namespace {

int g_failures = 0;

void Expect(const char *name, bool cond) {
    if (!cond) {
        g_failures++;
        printf("FAIL %s\n", name);
    }
}

// Quantised to 0.01 so compilers that fuse multiply-adds differently still agree.
void Mix(uint64_t &h, float v) {
    int64_t q = std::llround((double)v * 100.0);
    for (int i = 0; i < 8; i++) {
        h ^= (uint64_t)((q >> (i * 8)) & 0xFF);
        h *= 1099511628211ull;
    }
}

}  // namespace

int main() {
    const LevelDef &d = Level1Def();

    WorldState world;
    world.worldSize = d.terrain.worldSize;
    world.flatHalfWidth = d.terrain.flatHalfWidth;
    for (const ObstacleDef &o : d.hardObstacles) world.obstacles.push_back({{o.x, 0.0f, o.z}, o.radius, o.height, false});
    for (const ObstacleDef &o : d.softObstacles) world.obstacles.push_back({{o.x, o.height, o.z}, o.radius, 0.0f, true});

    GenerateScatter(world.scatter, world, d.scatter);
    uint64_t scatterHash = 14695981039346656037ull;
    for (const ScatterProp &p : world.scatter.props) {
        Mix(scatterHash, p.position.x);
        Mix(scatterHash, p.position.y);
        Mix(scatterHash, p.position.z);
        Mix(scatterHash, p.scale);
        Mix(scatterHash, p.yaw);
        Mix(scatterHash, (float)(int)p.kind);
        Mix(scatterHash, (float)p.tint);
    }

    CloudsState clouds;
    GenerateClouds(clouds, d.clouds);
    uint64_t cloudHash = 14695981039346656037ull;
    size_t puffTotal = 0;
    for (const Cloud &c : clouds.clouds) {
        Mix(cloudHash, c.position.x);
        Mix(cloudHash, c.position.y);
        Mix(cloudHash, c.position.z);
        Mix(cloudHash, c.boundRadius);
        for (const CloudPuff &p : c.puffs) {
            puffTotal++;
            Mix(cloudHash, p.offset.x);
            Mix(cloudHash, p.offset.y);
            Mix(cloudHash, p.offset.z);
            Mix(cloudHash, p.radii.x);
            Mix(cloudHash, p.radii.y);
            Mix(cloudHash, p.radii.z);
        }
    }

    printf("scatter props %zu hash %llu\n", world.scatter.props.size(), (unsigned long long)scatterHash);
    printf("clouds %zu puffs %zu hash %llu\n", clouds.clouds.size(), puffTotal, (unsigned long long)cloudHash);

    Expect("scatter layout unchanged", world.scatter.props.size() == 1800 && scatterHash == 15811973062304216539ull);
    Expect("cloud layout unchanged", clouds.clouds.size() == 30 && puffTotal == 114 && cloudHash == 16139608250772843788ull);

    if (g_failures == 0) printf("All scatter and cloud checks passed\n");
    return g_failures == 0 ? 0 : 1;
}
