#include "scatter.h"
#include "world.h"
#include "raymath.h"
#include "rlgl.h"
#include <cmath>
#include <cstdint>

namespace {

const int kMaxProps = 1800;
const float kCorridorMargin = 20.0f;
const float kObstacleClearance = 12.0f;
const float kSink = 0.4f;
const float kTreeHeight = 6.0f;  // AddTree's top at scale 1

uint32_t NextRandom(uint32_t &state) {
    state = state * 1664525u + 1013904223u;
    return state >> 8;
}

float Random01(uint32_t &state) {
    return (float)NextRandom(state) / (float)(1u << 24);
}

Color Shade(Color base, float k) {
    return (Color){(unsigned char)fminf(base.r * k, 255.0f), (unsigned char)fminf(base.g * k, 255.0f),
                   (unsigned char)fminf(base.b * k, 255.0f), 255};
}

// Face shaded from its outward normal; outward is decided against the prop centre so winding does not matter (culling is off).
void Face(Vector3 a, Vector3 b, Vector3 c, Vector3 center, Color base) {
    const Vector3 light = Vector3Normalize((Vector3){0.5f, 0.8f, 0.3f});
    Vector3 n = Vector3Normalize(Vector3CrossProduct(Vector3Subtract(b, a), Vector3Subtract(c, a)));
    Vector3 mid = Vector3Scale(Vector3Add(Vector3Add(a, b), c), 1.0f / 3.0f);
    if (Vector3DotProduct(n, Vector3Subtract(mid, center)) < 0.0f) n = Vector3Negate(n);
    Color col = Shade(base, 0.65f + 0.4f * fmaxf(Vector3DotProduct(n, light), 0.0f));
    rlColor4ub(col.r, col.g, col.b, 255);
    rlVertex3f(a.x, a.y, a.z);
    rlVertex3f(b.x, b.y, b.z);
    rlVertex3f(c.x, c.y, c.z);
}

Vector3 Ring(Vector3 origin, float radius, float angle, float y) {
    return (Vector3){origin.x + cosf(angle) * radius, origin.y + y, origin.z + sinf(angle) * radius};
}

void Cone(Vector3 origin, float baseY, float topY, float radius, int sides, float yaw, Vector3 center, Color col) {
    Vector3 apex = {origin.x, origin.y + topY, origin.z};
    for (int i = 0; i < sides; i++) {
        float a0 = yaw + 2.0f * PI * i / sides, a1 = yaw + 2.0f * PI * (i + 1) / sides;
        Face(Ring(origin, radius, a0, baseY), Ring(origin, radius, a1, baseY), apex, center, col);
    }
}

void AddTree(const ScatterProp &p) {
    float s = p.scale;
    Vector3 o = p.position;
    Vector3 center = {o.x, o.y + 2.5f * s, o.z};
    Color trunk = {110, 75, 40, 255};
    Color leaf = Shade((Color){40, 120, 50, 255}, 0.85f + p.tint / 850.0f);
    for (int i = 0; i < 4; i++) {
        float a0 = p.yaw + 2.0f * PI * i / 4, a1 = p.yaw + 2.0f * PI * (i + 1) / 4;
        Vector3 b0 = Ring(o, 0.3f * s, a0, 0.0f), b1 = Ring(o, 0.3f * s, a1, 0.0f);
        Vector3 t0 = Ring(o, 0.3f * s, a0, 1.4f * s), t1 = Ring(o, 0.3f * s, a1, 1.4f * s);
        Face(b0, b1, t1, center, trunk);
        Face(b0, t1, t0, center, trunk);
    }
    Cone(o, 1.0f * s, 4.0f * s, 1.9f * s, 6, p.yaw, center, leaf);
    Cone(o, 2.8f * s, 6.0f * s, 1.3f * s, 6, p.yaw + 0.5f, center, Shade(leaf, 1.08f));
}

void AddRock(const ScatterProp &p) {
    float s = p.scale;
    Vector3 o = p.position;
    Vector3 center = {o.x, o.y + 0.6f * s, o.z};
    Color col = Shade((Color){128, 126, 132, 255}, 0.8f + p.tint / 640.0f);
    Vector3 top = {o.x, o.y + 1.5f * s, o.z};
    Vector3 bottom = {o.x, o.y - 0.2f * s, o.z};
    Vector3 eq[4];
    const float radii[4] = {1.5f, 1.1f, 1.7f, 1.2f};
    for (int i = 0; i < 4; i++) eq[i] = Ring(o, radii[i] * s, p.yaw + PI * 0.5f * i, 0.5f * s);
    for (int i = 0; i < 4; i++) {
        Face(eq[i], eq[(i + 1) % 4], top, center, col);
        Face(eq[i], eq[(i + 1) % 4], bottom, center, col);
    }
}

}  // namespace

void GenerateScatter(ScatterState &scatter, const WorldState &world) {
    scatter.props.clear();
    uint32_t rng = 20240611u;
    float halfExtent = world.worldSize * 0.5f - 40.0f;
    float minX = world.flatHalfWidth + kCorridorMargin;

    int attempts = 0;
    while ((int)scatter.props.size() < kMaxProps && attempts < kMaxProps * 8) {
        attempts++;
        float x = (Random01(rng) * 2.0f - 1.0f) * halfExtent;
        float z = (Random01(rng) * 2.0f - 1.0f) * halfExtent;
        bool kindRoll = Random01(rng) < 0.3f;
        float scale = 1.2f + Random01(rng) * 1.2f;
        float yaw = Random01(rng) * 2.0f * PI;
        unsigned char tint = (unsigned char)(NextRandom(rng) & 0xFF);
        if (fabsf(x) < minX) continue;

        bool blocked = false;
        for (const Obstacle &o : world.obstacles) {
            float dx = x - o.position.x, dz = z - o.position.z;
            float r = o.radius + kObstacleClearance;
            if (dx * dx + dz * dz < r * r) { blocked = true; break; }
        }
        if (blocked) continue;

        ScatterProp p;
        p.position = (Vector3){x, GetGroundHeight(world, x, z) - kSink, z};
        p.scale = scale;
        p.yaw = yaw;
        p.kind = kindRoll ? ScatterKind::Rock : ScatterKind::Tree;
        p.tint = tint;
        scatter.props.push_back(p);
    }
}

void DrawPineTree(Vector3 base, float height, float yaw, unsigned char tint) {
    ScatterProp p = {base, height / kTreeHeight, yaw, ScatterKind::Tree, tint};
    rlDisableBackfaceCulling();
    rlCheckRenderBatchLimit(64);
    rlBegin(RL_TRIANGLES);
    AddTree(p);
    rlEnd();
    rlDrawRenderBatchActive();
    rlEnableBackfaceCulling();
}

void DrawScatter(const ScatterState &scatter, float density, Vector3 viewPosition, float drawDistance) {
    size_t count = (size_t)(Clamp(density, 0.0f, 1.0f) * (float)scatter.props.size());
    if (count == 0) return;
    float maxDistSq = drawDistance * drawDistance;

    rlDisableBackfaceCulling();
    for (size_t i = 0; i < count; i++) {
        const ScatterProp &p = scatter.props[i];
        float dx = p.position.x - viewPosition.x, dz = p.position.z - viewPosition.z;
        if (dx * dx + dz * dz > maxDistSq) continue;
        rlCheckRenderBatchLimit(64);
        rlBegin(RL_TRIANGLES);
        if (p.kind == ScatterKind::Tree) AddTree(p); else AddRock(p);
        rlEnd();
    }
    rlDrawRenderBatchActive();
    rlEnableBackfaceCulling();
}
