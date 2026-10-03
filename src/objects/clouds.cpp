#include "clouds.h"
#include "raymath.h"
#include "rlgl.h"
#include <cmath>
#include <cstdint>

namespace {

const int kMaxClouds = 30;
const int kSegments = 7;
const int kRings = 4;

uint32_t NextRandom(uint32_t &state) {
    state = state * 1664525u + 1013904223u;
    return state >> 8;
}

float Random01(uint32_t &state) {
    return (float)NextRandom(state) / (float)(1u << 24);
}

float RandomRange(uint32_t &state, float lo, float hi) {
    return lo + Random01(state) * (hi - lo);
}

Vector3 EllipsoidPoint(Vector3 center, Vector3 radii, int ring, int segment) {
    float phi = PI * ring / kRings;
    float theta = 2.0f * PI * segment / kSegments;
    return (Vector3){center.x + radii.x * sinf(phi) * cosf(theta), center.y + radii.y * cosf(phi),
                     center.z + radii.z * sinf(phi) * sinf(theta)};
}

// Outward is decided against the puff centre so winding does not matter (culling is off).
void Face(Vector3 a, Vector3 b, Vector3 c, Vector3 center) {
    const Vector3 light = Vector3Normalize((Vector3){0.5f, 0.8f, 0.3f});
    Vector3 n = Vector3Normalize(Vector3CrossProduct(Vector3Subtract(b, a), Vector3Subtract(c, a)));
    Vector3 mid = Vector3Scale(Vector3Add(Vector3Add(a, b), c), 1.0f / 3.0f);
    if (Vector3DotProduct(n, Vector3Subtract(mid, center)) < 0.0f) n = Vector3Negate(n);
    float k = 0.78f + 0.22f * Clamp(Vector3DotProduct(n, light) * 0.5f + 0.5f, 0.0f, 1.0f);
    rlColor4ub((unsigned char)(250.0f * k), (unsigned char)(252.0f * k), (unsigned char)(255.0f * k), 255);
    rlVertex3f(a.x, a.y, a.z);
    rlVertex3f(b.x, b.y, b.z);
    rlVertex3f(c.x, c.y, c.z);
}

void AddPuff(Vector3 center, Vector3 radii) {
    for (int r = 0; r < kRings; r++) {
        for (int s = 0; s < kSegments; s++) {
            Vector3 a = EllipsoidPoint(center, radii, r, s), b = EllipsoidPoint(center, radii, r, s + 1);
            Vector3 c = EllipsoidPoint(center, radii, r + 1, s), d = EllipsoidPoint(center, radii, r + 1, s + 1);
            if (r > 0) Face(a, b, c, center);
            if (r < kRings - 1) Face(b, d, c, center);
        }
    }
}

}  // namespace

void GenerateClouds(CloudsState &clouds) {
    clouds.clouds.clear();
    uint32_t rng = 7741903u;
    for (int i = 0; i < kMaxClouds; i++) {
        Cloud c;
        // Spread along the 1 km level (z) with some run-in/out, well above the flight corridor.
        c.position = (Vector3){RandomRange(rng, -260.0f, 260.0f), RandomRange(rng, 55.0f, 120.0f),
                               RandomRange(rng, -150.0f, 1150.0f)};
        float size = RandomRange(rng, 9.0f, 18.0f);
        int puffCount = 3 + (int)(Random01(rng) * 3.0f);
        float extent = 0.0f;
        for (int p = 0; p < puffCount; p++) {
            CloudPuff puff;
            float rx = size * RandomRange(rng, 0.6f, 1.1f);
            puff.radii = (Vector3){rx, rx * RandomRange(rng, 0.3f, 0.45f), rx * RandomRange(rng, 0.6f, 0.9f)};
            puff.offset = (Vector3){(p - (puffCount - 1) * 0.5f) * size * 0.9f, RandomRange(rng, -0.1f, 0.25f) * size,
                                    RandomRange(rng, -0.4f, 0.4f) * size};
            extent = fmaxf(extent, fabsf(puff.offset.x) + rx);
            c.puffs.push_back(puff);
        }
        c.boundRadius = extent + size;
        clouds.clouds.push_back(c);
    }
}

void DrawClouds(const CloudsState &clouds, int count, Vector3 viewPosition, float drawDistance) {
    size_t n = (size_t)Clamp((float)count, 0.0f, (float)clouds.clouds.size());
    if (n == 0) return;

    rlDisableBackfaceCulling();
    for (size_t i = 0; i < n; i++) {
        const Cloud &c = clouds.clouds[i];
        float reach = drawDistance + c.boundRadius;
        if (Vector3DistanceSqr(c.position, viewPosition) > reach * reach) continue;
        rlCheckRenderBatchLimit(2048);
        rlBegin(RL_TRIANGLES);
        for (const CloudPuff &p : c.puffs) AddPuff(Vector3Add(c.position, p.offset), p.radii);
        rlEnd();
    }
    rlDrawRenderBatchActive();
    rlEnableBackfaceCulling();
}
