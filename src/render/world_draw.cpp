#include "world_draw.h"
#include "ground_look.h"
#include "tree_look.h"
#include "sun.h"
#include "raymath.h"
#include "rlgl.h"
#include <cmath>

void DrawWorldObject(const WorldState &world, const GraphicsSettings &gfx, Vector3 viewPosition) {
    DrawGroundBackdrop();

    float halfSize = world.worldSize * 0.5f;
    DrawModel(world.terrainModel, (Vector3){-halfSize, 0.0f, -halfSize}, 1.0f, WHITE);

    bool cards = gfx.treeCards && TreeCardsAvailable();
    DrawScatter(world.scatter, gfx.scatterDensity, viewPosition, 450.0f, cards);
    if (cards) DrawTreeCards(world.scatter, gfx.scatterDensity, viewPosition, 450.0f);
    DrawClouds(world.clouds, gfx.cloudCount, viewPosition, 600.0f);

    if (gfx.distanceMarkers) {
        for (int i = 1; i <= 5; i++) {
            float z = i * 200.0f;
            DrawCylinder((Vector3){-15.0f, GetGroundHeight(world, -15.0f, z), z}, 0.3f, 0.3f, 4.0f, 8, DARKGRAY);
            DrawCylinder((Vector3){15.0f, GetGroundHeight(world, 15.0f, z), z}, 0.3f, 0.3f, 4.0f, 8, DARKGRAY);
        }
    }

    for (const Obstacle &o : world.obstacles) {
        if (o.soft) {
            float groundY = GetGroundHeight(world, o.position.x, o.position.z);
            float trunkTop = o.position.y - o.radius * 0.6f;
            if (trunkTop - groundY > 0.5f) {
                DrawCylinder((Vector3){o.position.x, groundY, o.position.z}, 0.35f, 0.45f, trunkTop - groundY, 6, (Color){110, 75, 40, 255});
            }
            DrawSphere(o.position, o.radius, (Color){40, 120, 50, 255});
            if (gfx.obstacleWires) DrawSphereWires(o.position, o.radius, 8, 8, (Color){20, 70, 30, 255});
        } else {
            DrawSphere(o.position, o.radius, (Color){180, 30, 30, 255});
            if (gfx.obstacleWires) DrawSphereWires(o.position, o.radius, 8, 8, BLACK);
        }
    }

    DrawRain(world.rain, ActiveRainDrops(world, gfx));
    DrawSnow(world.snow, ActiveSnowFlakes(world, gfx));
}

void DrawBlobShadow(const WorldState &world, Vector3 planePosition, float yawDegrees) {
    const float kMaxAltitude = 60.0f;
    const float kHalfAlong = 2.2f, kHalfAcross = 3.0f;
    const float kLift = 0.15f;
    const int kSegments = 20;

    float yaw = yawDegrees * DEG2RAD;
    Vector2 fwd = {sinf(yaw), cosf(yaw)};
    Vector2 right = {cosf(yaw), -sinf(yaw)};

    // Max over a few samples: GetGroundHeight is nearest-cell, the mesh is interpolated, so one sample can sink under it.
    float groundY = GetGroundHeight(world, planePosition.x, planePosition.z);
    for (int i = 0; i < 4; i++) {
        float sx = (i & 1 ? 1.0f : -1.0f) * kHalfAcross, sz = (i & 2 ? 1.0f : -1.0f) * kHalfAcross;
        groundY = fmaxf(groundY, GetGroundHeight(world, planePosition.x + sx, planePosition.z + sz));
    }

    float altitude = planePosition.y - groundY;
    if (altitude >= kMaxAltitude) return;
    float t = Clamp(altitude / kMaxAltitude, 0.0f, 1.0f);
    float scale = 1.0f + t * 1.5f;
    unsigned char alpha = (unsigned char)(110.0f * (1.0f - t));
    float y = groundY + kLift;

    rlDisableDepthMask();
    rlBegin(RL_TRIANGLES);
    rlColor4ub(0, 0, 0, alpha);
    for (int i = 0; i < kSegments; i++) {
        float a0 = 2.0f * PI * i / kSegments, a1 = 2.0f * PI * (i + 1) / kSegments;
        float p0a = cosf(a0) * kHalfAlong * scale, p0b = sinf(a0) * kHalfAcross * scale;
        float p1a = cosf(a1) * kHalfAlong * scale, p1b = sinf(a1) * kHalfAcross * scale;
        Vector3 c = {planePosition.x, y, planePosition.z};
        Vector3 v0 = {c.x + fwd.x * p0a + right.x * p0b, y, c.z + fwd.y * p0a + right.y * p0b};
        Vector3 v1 = {c.x + fwd.x * p1a + right.x * p1b, y, c.z + fwd.y * p1a + right.y * p1b};
        rlVertex3f(c.x, c.y, c.z); rlVertex3f(v0.x, v0.y, v0.z); rlVertex3f(v1.x, v1.y, v1.z);
        rlVertex3f(c.x, c.y, c.z); rlVertex3f(v1.x, v1.y, v1.z); rlVertex3f(v0.x, v0.y, v0.z);
    }
    rlEnd();
    rlDrawRenderBatchActive();
    rlEnableDepthMask();
}

namespace {

struct ShadowPoly { int count; Vector2 p[8]; };

// Local x is right, z is forward; pieces abut instead of overlapping so the translucent fill does not double up.
const ShadowPoly kShadowPieces[] = {
    {7, {{0.0f, 2.0f}, {0.45f, 1.5f}, {0.45f, -1.2f}, {0.2f, -2.3f}, {-0.2f, -2.3f}, {-0.45f, -1.2f}, {-0.45f, 1.5f}}},
    {4, {{0.45f, 0.7f}, {3.0f, 0.7f}, {3.0f, -0.6f}, {0.45f, -0.6f}}},
    {4, {{-0.45f, 0.7f}, {-0.45f, -0.6f}, {-3.0f, -0.6f}, {-3.0f, 0.7f}}},
    {4, {{0.45f, -1.7f}, {1.2f, -1.7f}, {1.2f, -2.3f}, {0.45f, -2.3f}}},
    {4, {{-0.45f, -1.7f}, {-0.45f, -2.3f}, {-1.2f, -2.3f}, {-1.2f, -1.7f}}},
};

}  // namespace

void DrawPlaneShadow(const WorldState &world, Vector3 planePosition, float yawDegrees, float pitchDegrees, float rollDegrees) {
    const float kMaxAltitude = 60.0f;
    const float kReach = 3.0f;
    const float kLift = 0.15f;

    // The shadow lands far to the side of the plane, where the terrain is not the terrain under the plane: find the
    // ground height at the landing spot (two passes, the spot depends on the height) and flatten the shadow there.
    auto highestNear = [&](float x, float z) {
        float h = GetGroundHeight(world, x, z);
        for (int i = 0; i < 4; i++) {
            float sx = (i & 1 ? 1.0f : -1.0f) * kReach, sz = (i & 2 ? 1.0f : -1.0f) * kReach;
            h = fmaxf(h, GetGroundHeight(world, x + sx, z + sz));
        }
        return h;
    };
    float groundY = highestNear(planePosition.x, planePosition.z);
    for (int pass = 0; pass < 2; pass++) {
        float reach = fmaxf(planePosition.y - groundY, 0.0f) / kSunDirection.y;
        groundY = highestNear(planePosition.x - kSunDirection.x * reach, planePosition.z - kSunDirection.z * reach);
    }

    float altitude = planePosition.y - groundY;
    if (altitude >= kMaxAltitude) return;
    float t = Clamp(altitude / kMaxAltitude, 0.0f, 1.0f);
    unsigned char alpha = (unsigned char)(120.0f * (1.0f - t));
    float y = groundY + kLift;

    auto project = [&](Vector2 local) {
        Vector3 v = {local.x, 0.0f, local.y};
        v = Vector3RotateByAxisAngle(v, {0.0f, 0.0f, 1.0f}, -rollDegrees * DEG2RAD);
        v = Vector3RotateByAxisAngle(v, {1.0f, 0.0f, 0.0f}, -pitchDegrees * DEG2RAD);
        v = Vector3RotateByAxisAngle(v, {0.0f, 1.0f, 0.0f}, yawDegrees * DEG2RAD);
        Vector3 w = Vector3Add(planePosition, v);
        float h = fmaxf(w.y - y, 0.0f) / kSunDirection.y;
        return Vector3{w.x - kSunDirection.x * h, y, w.z - kSunDirection.z * h};
    };

    rlDisableDepthMask();
    rlBegin(RL_TRIANGLES);
    rlColor4ub(0, 0, 0, alpha);
    for (const ShadowPoly &poly : kShadowPieces) {
        Vector3 first = project(poly.p[0]);
        for (int i = 1; i + 1 < poly.count; i++) {
            Vector3 a = project(poly.p[i]), b = project(poly.p[i + 1]);
            rlVertex3f(first.x, first.y, first.z); rlVertex3f(a.x, a.y, a.z); rlVertex3f(b.x, b.y, b.z);
            rlVertex3f(first.x, first.y, first.z); rlVertex3f(b.x, b.y, b.z); rlVertex3f(a.x, a.y, a.z);
        }
    }
    rlEnd();
    rlDrawRenderBatchActive();
    rlEnableDepthMask();
}
