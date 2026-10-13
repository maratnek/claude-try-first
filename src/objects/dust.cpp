#include "dust.h"
#include "sprites.h"
#include "raymath.h"
#include <cmath>

namespace {
const float kLifetime = 1.5f;
const float kRiseSpeed = 0.8f;
const float kDrag = 1.5f;

float Rand01(DustState &dust) {
    dust.seed = dust.seed * 1664525u + 1013904223u;
    return ((dust.seed >> 8) & 0xFFFFu) / 65535.0f;
}
}  // namespace

void ClearDust(DustState &dust) {
    for (int i = 0; i < kMaxDustPuffs; i++) dust.alive[i] = false;
    dust.head = 0;
}

int CountLiveDust(const DustState &dust) {
    int n = 0;
    for (int i = 0; i < kMaxDustPuffs; i++) n += dust.alive[i] ? 1 : 0;
    return n;
}

void SpawnDust(DustState &dust, Vector3 groundPoint, float strength01, int budget) {
    if (budget <= 0) return;
    if (budget > kMaxDustPuffs) budget = kMaxDustPuffs;
    float s = Clamp(strength01, 0.0f, 1.0f);
    int count = (int)(budget * s + 0.5f);
    if (count < 2) count = 2;
    for (int n = 0; n < count; n++) {
        float angle = Rand01(dust) * 2.0f * PI;
        float speed = (1.5f + 3.0f * Rand01(dust)) * (0.5f + s);
        DustPuff &p = dust.puffs[dust.head];
        p.position = {groundPoint.x, groundPoint.y + 0.2f, groundPoint.z};
        p.velocity = {cosf(angle) * speed, kRiseSpeed * (0.6f + 0.8f * Rand01(dust)), sinf(angle) * speed};
        p.age = 0.0f;
        p.lifetime = kLifetime * (0.8f + 0.4f * Rand01(dust));
        p.size = (0.5f + 0.5f * Rand01(dust)) * (0.5f + 0.9f * s);
        dust.alive[dust.head] = true;
        dust.head = (dust.head + 1) % kMaxDustPuffs;
    }
}

void UpdateDust(DustState &dust, float dt) {
    float damp = expf(-kDrag * dt);
    for (int i = 0; i < kMaxDustPuffs; i++) {
        if (!dust.alive[i]) continue;
        DustPuff &p = dust.puffs[i];
        p.age += dt;
        if (p.age >= p.lifetime) {
            dust.alive[i] = false;
            continue;
        }
        p.position = Vector3Add(p.position, Vector3Scale(p.velocity, dt));
        p.velocity.x *= damp;
        p.velocity.z *= damp;
    }
}

void DrawDust(const DustState &dust, const Camera3D &camera) {
    Vector3 forward = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
    Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, camera.up));
    BillboardAxes axes = {right, Vector3CrossProduct(right, forward)};

    BeginSprites(SpriteKind::Puff, false);
    for (int i = 0; i < kMaxDustPuffs; i++) {
        if (!dust.alive[i]) continue;
        const DustPuff &p = dust.puffs[i];
        float t = p.age / p.lifetime;
        float half = p.size * (0.8f + 2.2f * t);
        float fadeIn = fminf(t / 0.1f, 1.0f);
        unsigned char alpha = (unsigned char)(150.0f * fadeIn * (1.0f - t));
        DrawSprite(axes, p.position, half, i * 2.39996f + t * 0.6f, {150, 125, 95, alpha});
    }
    EndSprites();
}
