#include "exhaust.h"
#include "plane.h"
#include "sprites.h"
#include "raymath.h"

namespace {
const float kLifetime = 0.8f;
const float kBurstDuration = 0.7f;
const float kIdlePowerThreshold = 0.01f;
const float kIdleRateShare = 0.2f;
const float kSteadyBudgetShare = 0.6f;

// Plane space, metres: short stubs either side of the cowl (cowl radius ~0.5, centred y ~1.0, z ~2.1).
const Vector3 kExhaustPipes[2] = {{-0.55f, 0.85f, 1.8f}, {0.55f, 0.85f, 1.8f}};
const float kExhaustSpeed = 3.0f;
const float kExhaustSideSpeed = 0.6f;
const float kExhaustRiseSpeed = 0.6f;

float Rand01(ExhaustState &exhaust) {
    exhaust.seed = exhaust.seed * 1664525u + 1013904223u;
    return ((exhaust.seed >> 8) & 0xFFFFu) / 65535.0f;
}
}  // namespace

float ExhaustEmitRate(float enginePower, bool burst, int puffBudget) {
    if (puffBudget <= 0) return 0.0f;
    float power = Clamp(enginePower, 0.0f, 1.0f);
    float perSecond = puffBudget / kLifetime;
    if (burst) return perSecond;
    return perSecond * kSteadyBudgetShare * (kIdleRateShare + (1.0f - kIdleRateShare) * power);
}

void ClearExhaust(ExhaustState &exhaust) {
    for (int i = 0; i < kMaxExhaustPuffs; i++) exhaust.alive[i] = false;
    exhaust.head = 0;
    exhaust.nextPipe = 0;
    exhaust.spawnAccum = 0.0f;
    exhaust.burstRemaining = 0.0f;
    exhaust.prevPower = 0.0f;
    exhaust.started = false;
}

int ExhaustAliveCount(const ExhaustState &exhaust) {
    int count = 0;
    for (int i = 0; i < kMaxExhaustPuffs; i++) count += exhaust.alive[i] ? 1 : 0;
    return count;
}

void UpdateExhaust(ExhaustState &exhaust, const PlaneState &plane, bool engineRunning, int puffBudget, float dt) {
    if (puffBudget <= 0) {
        ClearExhaust(exhaust);
        return;
    }
    if (puffBudget > kMaxExhaustPuffs) puffBudget = kMaxExhaustPuffs;

    for (int i = 0; i < kMaxExhaustPuffs; i++) {
        if (!exhaust.alive[i]) continue;
        exhaust.puffs[i].age += dt;
        exhaust.puffs[i].position = Vector3Add(exhaust.puffs[i].position, Vector3Scale(exhaust.puffs[i].velocity, dt));
        if (exhaust.puffs[i].age >= kLifetime) exhaust.alive[i] = false;
    }

    if (!engineRunning) {
        exhaust.spawnAccum = 0.0f;
        exhaust.burstRemaining = 0.0f;
        exhaust.prevPower = 0.0f;
        return;
    }

    bool throttleUpOnGround = !plane.airborne && exhaust.prevPower <= kIdlePowerThreshold && plane.enginePower > kIdlePowerThreshold;
    if (!exhaust.started || throttleUpOnGround) exhaust.burstRemaining = kBurstDuration;
    exhaust.started = true;
    exhaust.prevPower = plane.enginePower;
    bool burst = exhaust.burstRemaining > 0.0f;
    exhaust.burstRemaining -= dt;

    // The ring only holds `puffBudget` puffs; a burst at full budget recycles the oldest, which is intended.
    exhaust.spawnAccum += ExhaustEmitRate(plane.enginePower, burst, puffBudget) * dt;
    while (exhaust.spawnAccum >= 1.0f) {
        exhaust.spawnAccum -= 1.0f;
        int slot = exhaust.head % puffBudget;
        Vector3 pipe = kExhaustPipes[exhaust.nextPipe];
        float side = pipe.x < 0.0f ? -1.0f : 1.0f;
        Vector3 localVelocity = {side * kExhaustSideSpeed * Rand01(exhaust), kExhaustRiseSpeed * (0.5f + Rand01(exhaust)), -kExhaustSpeed};
        exhaust.puffs[slot] = {Vector3Add(plane.position, PlaneToWorld(pipe, plane)), PlaneToWorld(localVelocity, plane), 0.0f,
                               Clamp(plane.enginePower, 0.0f, 1.0f), burst ? 1.0f : 0.0f};
        exhaust.alive[slot] = true;
        exhaust.head = (exhaust.head + 1) % puffBudget;
        exhaust.nextPipe = 1 - exhaust.nextPipe;
    }
}

void DrawExhaust(const ExhaustState &exhaust, const Camera3D &camera) {
    Vector3 forward = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
    Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, camera.up));
    BillboardAxes axes = {right, Vector3CrossProduct(right, forward)};

    BeginSprites(SpriteKind::Puff, false);
    for (int i = 0; i < kMaxExhaustPuffs; i++) {
        if (!exhaust.alive[i]) continue;
        const ExhaustPuff &p = exhaust.puffs[i];
        float t = p.age / kLifetime;
        float half = (0.25f + 0.5f * p.power + 0.25f * p.burst + t * (0.7f + 0.5f * p.power)) * (1.0f + 0.4f * p.burst);
        float density = 0.25f + 0.55f * p.power + 0.3f * p.burst;
        float fadeIn = fminf(t / 0.1f, 1.0f);
        unsigned char alpha = (unsigned char)(150.0f * fminf(density, 1.0f) * fadeIn * (1.0f - t));
        unsigned char shade = (unsigned char)(150.0f - 60.0f * p.power - 30.0f * p.burst);
        DrawSprite(axes, p.position, half, i * 2.39996f + t * 0.8f, {shade, shade, shade, alpha});
    }
    EndSprites();
}
