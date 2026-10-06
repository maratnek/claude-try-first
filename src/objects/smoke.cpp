#include "smoke.h"
#include "raymath.h"
#include "rlgl.h"
#include <cmath>

namespace {
const float kLifetime = 1.5f;
const float kRiseSpeed = 2.0f;
const float kTailOffset = 2.5f;
const float kDustThreshold = 0.05f;

float Rand01(SmokeState &smoke) {
    smoke.seed = smoke.seed * 1664525u + 1013904223u;
    return ((smoke.seed >> 8) & 0xFFFFu) / 65535.0f;
}

void AgePuffs(SmokeState &smoke, float dt) {
    for (int i = 0; i < kMaxSmokePuffs; i++) {
        if (!smoke.alive[i]) continue;
        smoke.puffs[i].age += dt;
        smoke.puffs[i].position.y += smoke.riseSpeed * dt;
        if (smoke.puffs[i].age >= smoke.lifetime) smoke.alive[i] = false;
    }
}
}  // namespace

void ClearSmoke(SmokeState &smoke) {
    for (int i = 0; i < kMaxSmokePuffs; i++) smoke.alive[i] = false;
    smoke.head = 0;
    smoke.spawnTimer = 0.0f;
    smoke.mode = SmokeMode::Trail;
    smoke.lifetime = kLifetime;
    smoke.riseSpeed = kRiseSpeed;
    smoke.sizeScale = 1.0f;
    smoke.darkness = 0.0f;
    smoke.emitRemaining = 0.0f;
    smoke.puffRate = 0;
}

void UpdateSmoke(SmokeState &smoke, Vector3 planePosition, Vector3 planeForward, bool emitting, int puffCount, float dt) {
    if (puffCount <= 0) {
        ClearSmoke(smoke);
        return;
    }
    if (puffCount > kMaxSmokePuffs) puffCount = kMaxSmokePuffs;

    AgePuffs(smoke, dt);

    if (!emitting) {
        smoke.spawnTimer = 0.0f;
        return;
    }
    float interval = kLifetime / puffCount;
    smoke.spawnTimer += dt;
    while (smoke.spawnTimer >= interval) {
        smoke.spawnTimer -= interval;
        smoke.puffs[smoke.head] = {Vector3Subtract(planePosition, Vector3Scale(planeForward, kTailOffset)), smoke.spawnTimer};
        smoke.alive[smoke.head] = true;
        smoke.head = (smoke.head + 1) % kMaxSmokePuffs;
    }
}

void StartExplosionSmoke(SmokeState &smoke, Vector3 origin, float s, int puffBudget) {
    ClearSmoke(smoke);
    if (puffBudget <= 0) return;
    if (puffBudget > kMaxSmokePuffs) puffBudget = kMaxSmokePuffs;
    smoke.mode = SmokeMode::ExplosionSmoke;
    smoke.origin = origin;
    smoke.seed = 0x5a0c3u;
    if (s < kDustThreshold) {
        smoke.lifetime = 1.2f;
        smoke.riseSpeed = 1.0f;
        smoke.sizeScale = 0.8f;
        smoke.darkness = 0.0f;
        smoke.spread = 0.8f;
        smoke.emitRemaining = 0.4f;
        smoke.puffRate = 12;
        return;
    }
    smoke.lifetime = 2.5f;
    smoke.riseSpeed = 3.0f + 2.0f * s;
    smoke.sizeScale = 0.8f + 1.2f * s;
    smoke.darkness = 1.0f;
    smoke.spread = 0.6f + 1.4f * s;
    smoke.emitRemaining = 2.0f + 6.0f * s;
    smoke.puffRate = (int)fmaxf(4.0f, puffBudget * (0.3f + 0.7f * s));
}

void UpdateExplosionSmoke(SmokeState &smoke, float dt) {
    if (smoke.mode != SmokeMode::ExplosionSmoke) return;
    AgePuffs(smoke, dt);
    if (smoke.emitRemaining <= 0.0f) return;
    smoke.emitRemaining -= dt;
    float interval = smoke.lifetime / smoke.puffRate;
    smoke.spawnTimer += dt;
    while (smoke.spawnTimer >= interval) {
        smoke.spawnTimer -= interval;
        Vector3 jitter = {(Rand01(smoke) - 0.5f) * 2.0f * smoke.spread, Rand01(smoke) * smoke.spread * 0.5f, (Rand01(smoke) - 0.5f) * 2.0f * smoke.spread};
        smoke.puffs[smoke.head] = {Vector3Add(smoke.origin, jitter), smoke.spawnTimer};
        smoke.alive[smoke.head] = true;
        smoke.head = (smoke.head + 1) % kMaxSmokePuffs;
    }
}

void DrawSmoke(const SmokeState &smoke, const Camera3D &camera) {
    Vector3 forward = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
    Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, camera.up));
    Vector3 up = Vector3CrossProduct(right, forward);

    rlDisableDepthMask();
    rlDisableBackfaceCulling();
    for (int i = 0; i < kMaxSmokePuffs; i++) {
        if (!smoke.alive[i]) continue;
        rlCheckRenderBatchLimit(6);
        rlBegin(RL_TRIANGLES);
        float t = smoke.puffs[i].age / smoke.lifetime;
        float half = (0.4f + t * 1.4f) * smoke.sizeScale;
        unsigned char shade = (unsigned char)((70.0f + t * 90.0f) * (1.0f - 0.65f * smoke.darkness * (1.0f - t)));
        unsigned char alpha = (unsigned char)(150.0f * (1.0f - t));
        Vector3 c = smoke.puffs[i].position;
        Vector3 r = Vector3Scale(right, half), u = Vector3Scale(up, half);
        Vector3 a = Vector3Subtract(Vector3Subtract(c, r), u);
        Vector3 b = Vector3Subtract(Vector3Add(c, r), u);
        Vector3 d = Vector3Add(Vector3Add(c, r), u);
        Vector3 e = Vector3Add(Vector3Subtract(c, r), u);
        rlColor4ub(shade, shade, shade, alpha);
        rlVertex3f(a.x, a.y, a.z);
        rlVertex3f(b.x, b.y, b.z);
        rlVertex3f(d.x, d.y, d.z);
        rlVertex3f(a.x, a.y, a.z);
        rlVertex3f(d.x, d.y, d.z);
        rlVertex3f(e.x, e.y, e.z);
        rlEnd();
    }
    rlDrawRenderBatchActive();
    rlEnableBackfaceCulling();
    rlEnableDepthMask();
}
