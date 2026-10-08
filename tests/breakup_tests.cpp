#include "objects/debris.h"
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

namespace {

constexpr float kDt = 1.0f / 60.0f;
constexpr float kMaxSettleTime = 12.0f;
constexpr float kEpsilon = 0.01f;

int g_failures = 0;
bool g_verbose = false;

enum class Terrain { Flat, Slope, Steps };
Terrain g_terrain = Terrain::Flat;

void Expect(const char *name, bool cond) {
    if (!cond) g_failures++;
    if (!cond || g_verbose) printf("%s %s\n", cond ? "ok  " : "FAIL", name);
}

struct Box {
    Vector3 lo, hi;
};

struct Scene {
    PlaneModel model;
    std::vector<Mesh> meshes;
    std::vector<std::vector<float>> vertices;
};

void AddMesh(Scene &s, Box box, int group, int part) {
    std::vector<float> v;
    for (int c = 0; c < 8; c++) {
        v.push_back(c & 1 ? box.hi.x : box.lo.x);
        v.push_back(c & 2 ? box.hi.y : box.lo.y);
        v.push_back(c & 4 ? box.hi.z : box.lo.z);
    }
    s.vertices.push_back(v);
    Mesh mesh = {};
    mesh.vertexCount = 8;
    s.meshes.push_back(mesh);
    s.model.breakData.meshGroup.push_back(group);
    s.model.meshPart.push_back(part);
    s.model.meshIsBlur.push_back(false);
}

void Finish(Scene &s) {
    for (size_t i = 0; i < s.meshes.size(); i++) s.meshes[i].vertices = s.vertices[i].data();
    s.model.model.meshCount = (int)s.meshes.size();
    s.model.model.meshes = s.meshes.data();
    s.model.animated = true;
}

void AddGroup(BreakData &data, const char *name, Box box, float mass, float strength, int parent, Vector3 joint) {
    BreakGroup g;
    g.name = name;
    g.mass = mass;
    g.strength = strength;
    g.parent = parent;
    g.joint = joint;
    g.meshCount = 1;
    g.boundsMin = box.lo;
    g.boundsMax = box.hi;
    g.centreOfMass = {(box.lo.x + box.hi.x) * 0.5f, (box.lo.y + box.hi.y) * 0.5f, (box.lo.z + box.hi.z) * 0.5f};
    data.groups.push_back(g);
}

void BuildGroupedPlane(Scene &s) {
    const Box boxes[] = {
        {{-0.5f, 0.2f, -2.0f}, {0.5f, 1.3f, 2.0f}},
        {{-5.0f, 1.4f, -0.8f}, {-0.5f, 1.6f, 0.8f}},
        {{0.5f, 1.4f, -0.8f}, {5.0f, 1.6f, 0.8f}},
        {{-0.1f, 0.8f, -5.0f}, {0.1f, 2.2f, -2.0f}},
        {{-1.4f, -0.2f, 0.6f}, {-1.0f, 0.5f, 1.4f}},
        {{1.0f, -0.2f, 0.6f}, {1.4f, 0.5f, 1.4f}},
        {{-0.1f, 0.4f, 2.0f}, {0.1f, 1.2f, 2.4f}},
    };
    const char *names[] = {"hull", "wing_l", "wing_r", "tail", "wheel_l", "wheel_r", "prop"};
    const float mass[] = {60.0f, 12.0f, 12.0f, 6.0f, 4.0f, 4.0f, 3.0f};
    const float strength[] = {0.0f, 60.0f, 60.0f, 40.0f, 25.0f, 25.0f, 15.0f};
    BreakData &data = s.model.breakData;
    for (int i = 0; i < 7; i++) {
        Vector3 joint = {(boxes[i].lo.x + boxes[i].hi.x) * 0.5f, boxes[i].lo.y, (boxes[i].lo.z + boxes[i].hi.z) * 0.5f};
        if (i == 1) joint.x = boxes[i].hi.x;
        if (i == 2) joint.x = boxes[i].lo.x;
        AddGroup(data, names[i], boxes[i], mass[i], strength[i], i == 0 ? -1 : 0, joint);
        AddMesh(s, boxes[i], i, -1);
    }
    data.valid = true;
    data.hull = 0;
    Finish(s);
}

void BuildFixedPlane(Scene &s) {
    const PlanePart parts[] = {PART_PROPELLER, PART_RUDDER, PART_ELEVATOR, PART_AILERON_RIGHT, PART_AILERON_LEFT, PART_WHEEL_RIGHT, PART_WHEEL_LEFT};
    const Box boxes[] = {
        {{-0.05f, 0.5f, 2.0f}, {0.05f, 1.3f, 2.2f}},
        {{-0.05f, 0.8f, -4.0f}, {0.05f, 2.0f, -3.0f}},
        {{-1.5f, 0.8f, -4.0f}, {1.5f, 0.9f, -3.2f}},
        {{2.0f, 1.4f, -0.5f}, {5.0f, 1.5f, 0.5f}},
        {{-5.0f, 1.4f, -0.5f}, {-2.0f, 1.5f, 0.5f}},
        {{1.0f, -0.2f, 0.6f}, {1.4f, 0.5f, 1.4f}},
        {{-1.4f, -0.2f, 0.6f}, {-1.0f, 0.5f, 1.4f}},
    };
    AddMesh(s, {{-0.5f, 0.2f, -2.0f}, {0.5f, 1.3f, 2.0f}}, 0, -1);
    for (int i = 0; i < 7; i++) {
        AddMesh(s, boxes[i], 0, parts[i]);
        s.model.parts[parts[i]].pivot = {(boxes[i].lo.x + boxes[i].hi.x) * 0.5f, (boxes[i].lo.y + boxes[i].hi.y) * 0.5f, (boxes[i].lo.z + boxes[i].hi.z) * 0.5f};
    }
    Finish(s);
}

struct Crash {
    const char *name;
    float speed;
    float pitch;
    float roll;
    float yaw;
    bool ground;
    int pieces;
};

void RunCrash(const Scene &scene, const Crash &c, Terrain terrain, const char *label) {
    g_terrain = terrain;
    WorldState world;
    PlaneState plane;
    plane.position = {12.0f, 0.0f, 30.0f};
    plane.position.y = GetGroundHeight(world, plane.position.x, plane.position.z) + 1.0f;
    plane.pitch = c.pitch;
    plane.roll = c.roll;
    plane.yaw = c.yaw;
    plane.speed = c.speed;

    Vector3 forward = PlaneToWorld({0.0f, 0.0f, 1.0f}, plane);
    Vector3 velocity = {forward.x * c.speed, forward.y * c.speed, forward.z * c.speed};
    ImpactInfo impact = {plane.position, velocity, c.speed, c.ground, 1.5f};

    DebrisState debris;
    SpawnDebris(debris, scene.model, plane, impact, c.pieces);

    char name[160];
    snprintf(name, sizeof(name), "%s %s: spawns pieces", label, c.name);
    Expect(name, debris.count > 0);

    float elapsed = 0.0f;
    bool allSettled = false;
    while (!allSettled && elapsed < kMaxSettleTime) {
        UpdateDebris(debris, world, kDt);
        elapsed += kDt;
        allSettled = true;
        for (int i = 0; i < debris.count; i++) allSettled = allSettled && debris.pieces[i].settled;
    }
    snprintf(name, sizeof(name), "%s %s: all pieces settle", label, c.name);
    Expect(name, allSettled);

    float lowest = 1e9f;
    for (int i = 0; i < debris.count; i++) lowest = fminf(lowest, DebrisLowestClearance(debris.pieces[i], world));
    snprintf(name, sizeof(name), "%s %s: lowest clearance %.3f >= -%.2f", label, c.name, lowest, kEpsilon);
    Expect(name, lowest >= -kEpsilon);
}

}  // namespace

float GetGroundHeight(const WorldState &, float worldX, float worldZ) {
    switch (g_terrain) {
        case Terrain::Flat: return 0.0f;
        case Terrain::Slope: return 0.15f * worldX + 0.05f * worldZ;
        case Terrain::Steps: return 1.5f * floorf(worldX / 6.0f) + 0.8f * floorf(worldZ / 9.0f);
    }
    return 0.0f;
}

int main(int argc, char **) {
    g_verbose = argc > 1;
    SetTraceLogLevel(LOG_NONE);

    Scene grouped, fixed;
    BuildGroupedPlane(grouped);
    BuildFixedPlane(fixed);

    const Crash crashes[] = {
        {"slow nose-down ground", 12.0f, -20.0f, 0.0f, 0.0f, true, 4},
        {"fast dive ground", 50.0f, -60.0f, 0.0f, 35.0f, true, 6},
        {"level obstacle hit", 40.0f, 0.0f, 0.0f, 90.0f, false, 12},
        {"rolled wing-first", 30.0f, -10.0f, 70.0f, -40.0f, true, 6},
        {"inverted stall", 18.0f, 20.0f, 170.0f, 200.0f, true, 12},
        {"max speed head-on", 70.0f, -5.0f, 15.0f, 10.0f, false, 12},
    };
    const struct { Terrain terrain; const char *label; } terrains[] = {{Terrain::Flat, "flat"}, {Terrain::Slope, "slope"}, {Terrain::Steps, "steps"}};

    for (const auto &t : terrains) {
        for (const Crash &c : crashes) {
            RunCrash(grouped, c, t.terrain, (std::string("grouped ") + t.label).c_str());
            RunCrash(fixed, c, t.terrain, (std::string("fixed ") + t.label).c_str());
        }
    }

    if (g_failures) printf("%d check(s) failed\n", g_failures);
    else printf("all breakup checks passed\n");
    return g_failures ? 1 : 0;
}
