#include "explosion.h"
#include "raymath.h"
#include "rlgl.h"
#include <cmath>

namespace {
constexpr float kNoFireBelow = 0.05f;
constexpr float kGrowSeconds = 0.4f;
constexpr float kFadeSeconds = 1.2f;
constexpr float kFlashSeconds = 0.15f;
constexpr float kFlashAlpha = 0.35f;
constexpr float kBigRadius = 6.0f;
constexpr float kMinRadius = 0.8f;
constexpr float kRingSeconds = 0.3f;
constexpr int kRingSegments = 24;

struct Ball {
    Vector3 offset;  // fraction of the fireball radius
    float size;
    float delay;
    Color hot;
    Color cold;
};

const Ball kBalls[3] = {
    {{0.0f, 0.0f, 0.0f}, 1.0f, 0.0f, {255, 150, 40, 255}, {70, 45, 30, 255}},
    {{0.5f, 0.3f, -0.3f}, 0.7f, 0.08f, {255, 200, 70, 255}, {60, 40, 30, 255}},
    {{-0.4f, 0.6f, 0.4f}, 0.8f, 0.16f, {200, 70, 20, 255}, {40, 30, 25, 255}},
};

unsigned char Lerp8(unsigned char a, unsigned char b, float t) {
    return (unsigned char)(a + (b - a) * t);
}

void DrawRingSprite(Vector3 centre, float outer, float inner, Color color, const Camera3D &camera) {
    Vector3 forward = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
    Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, camera.up));
    Vector3 up = Vector3CrossProduct(right, forward);
    rlCheckRenderBatchLimit(kRingSegments * 6);
    rlBegin(RL_TRIANGLES);
    rlColor4ub(color.r, color.g, color.b, color.a);
    for (int i = 0; i < kRingSegments; i++) {
        float a0 = 2.0f * PI * i / kRingSegments, a1 = 2.0f * PI * (i + 1) / kRingSegments;
        Vector3 d0 = Vector3Add(Vector3Scale(right, cosf(a0)), Vector3Scale(up, sinf(a0)));
        Vector3 d1 = Vector3Add(Vector3Scale(right, cosf(a1)), Vector3Scale(up, sinf(a1)));
        Vector3 o0 = Vector3Add(centre, Vector3Scale(d0, outer)), o1 = Vector3Add(centre, Vector3Scale(d1, outer));
        Vector3 i0 = Vector3Add(centre, Vector3Scale(d0, inner)), i1 = Vector3Add(centre, Vector3Scale(d1, inner));
        rlVertex3f(o0.x, o0.y, o0.z);
        rlVertex3f(i0.x, i0.y, i0.z);
        rlVertex3f(o1.x, o1.y, o1.z);
        rlVertex3f(o1.x, o1.y, o1.z);
        rlVertex3f(i0.x, i0.y, i0.z);
        rlVertex3f(i1.x, i1.y, i1.z);
    }
    rlEnd();
}
}  // namespace

void ClearExplosion(ExplosionState &explosion) {
    explosion = ExplosionState{};
}

void StartExplosion(ExplosionState &explosion, Vector3 centre, float s, int fireballs, bool flash) {
    ClearExplosion(explosion);
    if (s < kNoFireBelow) return;
    explosion.active = true;
    explosion.scale = fminf(s, 1.0f);
    explosion.centre = centre;
    explosion.fireballs = fireballs > 3 ? 3 : fireballs;
    explosion.flash = flash;
}

void UpdateExplosion(ExplosionState &explosion, float dt) {
    if (!explosion.active) return;
    explosion.age += dt;
    float life = explosion.fireballs > 0 ? kGrowSeconds + kFadeSeconds + 0.16f : kRingSeconds;
    if (explosion.age > life) explosion.active = false;
}

void DrawExplosion(const ExplosionState &explosion, const Camera3D &camera) {
    if (!explosion.active) return;
    float s = explosion.scale;
    rlDisableDepthMask();
    rlDisableBackfaceCulling();
    if (explosion.fireballs == 0) {
        float t = explosion.age / kRingSeconds;
        float radius = (1.5f + 3.5f * s) * (0.4f + 0.6f * t);
        Color color = {255, 140, 30, (unsigned char)(220.0f * (1.0f - t))};
        DrawRingSprite(explosion.centre, radius, radius * 0.7f, color, camera);
    } else {
        float radius = fmaxf(kBigRadius * s, kMinRadius);
        for (int i = 0; i < explosion.fireballs; i++) {
            const Ball &ball = kBalls[i];
            float age = explosion.age - ball.delay;
            if (age < 0.0f) continue;
            float grow = fminf(age / kGrowSeconds, 1.0f);
            float fade = fmaxf((age - kGrowSeconds) / kFadeSeconds, 0.0f);
            if (fade >= 1.0f) continue;
            float r = radius * ball.size * (1.0f - (1.0f - grow) * (1.0f - grow));
            float cool = fminf(age / (kGrowSeconds + kFadeSeconds), 1.0f);
            Color color = {Lerp8(ball.hot.r, ball.cold.r, cool), Lerp8(ball.hot.g, ball.cold.g, cool),
                           Lerp8(ball.hot.b, ball.cold.b, cool), (unsigned char)(230.0f * (1.0f - fade))};
            Vector3 centre = Vector3Add(explosion.centre, Vector3Scale(ball.offset, radius * grow));
            DrawSphereEx(centre, r, 8, 12, color);
            rlDrawRenderBatchActive();
        }
    }
    rlEnableBackfaceCulling();
    rlEnableDepthMask();
}

void DrawExplosionFlash(const ExplosionState &explosion) {
    if (!explosion.active || !explosion.flash || explosion.age >= kFlashSeconds) return;
    float alpha = kFlashAlpha * explosion.scale * (1.0f - explosion.age / kFlashSeconds);
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(WHITE, alpha));
}
