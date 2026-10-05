#include "smoke.h"
#include "raymath.h"
#include "rlgl.h"
#include <cmath>

namespace {
const float kLifetime = 1.5f;
const float kRiseSpeed = 2.0f;
const float kTailOffset = 2.5f;
}  // namespace

void ClearSmoke(SmokeState &smoke) {
    for (int i = 0; i < kMaxSmokePuffs; i++) smoke.alive[i] = false;
    smoke.head = 0;
    smoke.spawnTimer = 0.0f;
}

void UpdateSmoke(SmokeState &smoke, Vector3 planePosition, Vector3 planeForward, bool emitting, int puffCount, float dt) {
    if (puffCount <= 0) {
        ClearSmoke(smoke);
        return;
    }
    if (puffCount > kMaxSmokePuffs) puffCount = kMaxSmokePuffs;

    for (int i = 0; i < kMaxSmokePuffs; i++) {
        if (!smoke.alive[i]) continue;
        smoke.puffs[i].age += dt;
        smoke.puffs[i].position.y += kRiseSpeed * dt;
        if (smoke.puffs[i].age >= kLifetime) smoke.alive[i] = false;
    }

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
        float t = smoke.puffs[i].age / kLifetime;
        float half = 0.4f + t * 1.4f;
        unsigned char shade = (unsigned char)(70.0f + t * 90.0f);
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
