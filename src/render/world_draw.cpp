#include "world_draw.h"
#include "ground_look.h"
#include "raymath.h"
#include "rlgl.h"
#include <cmath>

void DrawWorldObject(const WorldState &world, const GraphicsSettings &gfx, Vector3 viewPosition) {
    DrawGroundBackdrop();

    float halfSize = world.worldSize * 0.5f;
    DrawModel(world.terrainModel, (Vector3){-halfSize, 0.0f, -halfSize}, 1.0f, WHITE);

    DrawScatter(world.scatter, gfx.scatterDensity, viewPosition, 450.0f);
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
