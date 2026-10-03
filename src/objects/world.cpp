#include "world.h"
#include "raymath.h"
#include "rlgl.h"
#include <cmath>

namespace {

float SmoothStep01(float t) {
    t = Clamp(t, 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

float ComputeRawHeight(float x, float z, float maxHeight, float flatHalfWidth, float u, float v) {
    float h = 0.5f + 0.25f * sinf(x * 0.015f) + 0.25f * cosf(z * 0.012f) +
               0.12f * sinf(x * 0.05f + z * 0.04f);
    h = Clamp(h, 0.0f, 1.0f) * maxHeight;

    // Keep a flat flight corridor around x=0 regardless of z, so the runway
    // and straight-line flight path stay clear of hills.
    h *= SmoothStep01(fabsf(x) / flatHalfWidth);

    // Fade height back to 0 near the heightmap's outer edge so this finite
    // patch blends into the flat backdrop plane instead of showing as a
    // raised square with a visible cliff around it.
    float edgeDist = fminf(fminf(u, 1.0f - u), fminf(v, 1.0f - v));
    h *= SmoothStep01(edgeDist / 0.12f);

    return h;
}

Color LerpColor(Color a, Color b, float t) {
    return (Color){(unsigned char)(a.r + (b.r - a.r) * t), (unsigned char)(a.g + (b.g - a.g) * t),
                   (unsigned char)(a.b + (b.b - a.b) * t), 255};
}

const Color kGrass = {80, 150, 90, 255};

// Flat normal shades to exactly 1 so flat low ground matches the backdrop plane colour.
Color TerrainVertexColor(float heightNorm, Vector3 normal) {
    const Color dirt = {146, 120, 82, 255};
    const Color rock = {128, 126, 132, 255};
    const Vector3 light = Vector3Normalize((Vector3){0.5f, 0.8f, 0.3f});

    float slope = 1.0f - Clamp(normal.y, 0.0f, 1.0f);
    float dirtW = fmaxf(0.85f * SmoothStep01((heightNorm - 0.55f) / 0.4f), SmoothStep01((slope - 0.025f) / 0.06f));
    float rockW = fmaxf(SmoothStep01((heightNorm - 0.88f) / 0.12f), SmoothStep01((slope - 0.09f) / 0.08f));

    Color c = LerpColor(LerpColor(kGrass, dirt, dirtW), rock, rockW);
    float shade = Clamp(0.7f + 0.3f * Vector3DotProduct(normal, light) / light.y, 0.6f, 1.1f);
    c.r = (unsigned char)fminf(c.r * shade, 255.0f);
    c.g = (unsigned char)fminf(c.g * shade, 255.0f);
    c.b = (unsigned char)fminf(c.b * shade, 255.0f);
    return c;
}

}  // namespace

void ApplyTerrainColors(WorldState &world, const GraphicsSettings &gfx) {
    if (world.terrainColored == gfx.terrainColors) return;
    const std::vector<unsigned char> &src = gfx.terrainColors ? world.terrainShadedColors : world.terrainFlatColors;
    UpdateMeshBuffer(world.terrainModel.meshes[0], 3, src.data(), (int)src.size(), 0);
    world.terrainColored = gfx.terrainColors;
}

void GenerateWorld(WorldState &world, const GraphicsSettings &gfx) {
    world.heights.assign(world.gridSize * world.gridSize, 0.0f);

    Image heightImage = GenImageColor(world.gridSize, world.gridSize, BLACK);
    float halfSize = world.worldSize * 0.5f;

    for (int j = 0; j < world.gridSize; j++) {
        for (int i = 0; i < world.gridSize; i++) {
            float u = (float)i / (float)(world.gridSize - 1);
            float v = (float)j / (float)(world.gridSize - 1);
            float x = -halfSize + u * world.worldSize;
            float z = -halfSize + v * world.worldSize;

            float h = ComputeRawHeight(x, z, world.maxHeight, world.flatHalfWidth, u, v);
            world.heights[j * world.gridSize + i] = h;

            unsigned char gray = (unsigned char)Clamp((h / world.maxHeight) * 255.0f, 0.0f, 255.0f);
            ImageDrawPixel(&heightImage, i, j, (Color){gray, gray, gray, 255});
        }
    }

    Mesh terrainMesh = GenMeshHeightmap(heightImage, (Vector3){world.worldSize, world.maxHeight, world.worldSize});
    UnloadImage(heightImage);

    // GenMeshHeightmap already uploaded the mesh without a colour attribute; re-upload with one.
    rlUnloadVertexArray(terrainMesh.vaoId);
    for (int b = 0; b < 7; b++) rlUnloadVertexBuffer(terrainMesh.vboId[b]);
    terrainMesh.vaoId = 0;
    RL_FREE(terrainMesh.vboId);
    terrainMesh.vboId = nullptr;

    int colorBytes = terrainMesh.vertexCount * 4;
    world.terrainShadedColors.assign(colorBytes, 255);
    world.terrainFlatColors.assign(colorBytes, 255);
    for (int v = 0; v < terrainMesh.vertexCount; v++) {
        Vector3 n = {terrainMesh.normals[v * 3], terrainMesh.normals[v * 3 + 1], terrainMesh.normals[v * 3 + 2]};
        Color c = TerrainVertexColor(terrainMesh.vertices[v * 3 + 1] / world.maxHeight, n);
        world.terrainShadedColors[v * 4] = c.r;
        world.terrainShadedColors[v * 4 + 1] = c.g;
        world.terrainShadedColors[v * 4 + 2] = c.b;
        world.terrainFlatColors[v * 4] = kGrass.r;
        world.terrainFlatColors[v * 4 + 1] = kGrass.g;
        world.terrainFlatColors[v * 4 + 2] = kGrass.b;
    }
    const std::vector<unsigned char> &initial = gfx.terrainColors ? world.terrainShadedColors : world.terrainFlatColors;
    world.terrainColored = gfx.terrainColors;
    terrainMesh.colors = (unsigned char *)MemAlloc(colorBytes);
    for (int i = 0; i < colorBytes; i++) terrainMesh.colors[i] = initial[i];
    UploadMesh(&terrainMesh, false);

    world.terrainModel = LoadModelFromMesh(terrainMesh);
    world.terrainModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = WHITE;

    world.obstacles.clear();
    float obstacleZs[] = {300.0f, 500.0f, 700.0f, 850.0f};
    float obstacleXs[] = {-8.0f, 10.0f, -6.0f, 9.0f};
    for (int i = 0; i < 4; i++) AddHardObstacle(world, obstacleXs[i], obstacleZs[i], 6.0f, 3.0f);

    AddSoftObstacle(world, 9.0f, 120.0f, 0.8f, 1.6f);
    AddSoftObstacle(world, -4.0f, 220.0f, 9.0f, 2.5f);
    AddSoftObstacle(world, 6.0f, 420.0f, 14.0f, 2.5f);
    AddSoftObstacle(world, -5.0f, 620.0f, 10.0f, 2.5f);

    GenerateScatter(world.scatter, world);
    GenerateClouds(world.clouds);
}

void AddHardObstacle(WorldState &world, float x, float z, float height, float radius) {
    Obstacle o;
    o.position = (Vector3){x, GetGroundHeight(world, x, z) + height, z};
    o.radius = radius;
    world.obstacles.push_back(o);
}

void AddSoftObstacle(WorldState &world, float x, float z, float height, float radius) {
    Obstacle o;
    o.position = (Vector3){x, GetGroundHeight(world, x, z) + height, z};
    o.radius = radius;
    o.soft = true;
    world.obstacles.push_back(o);
}

void DrawWorldObject(const WorldState &world, const GraphicsSettings &gfx, Vector3 viewPosition) {
    // Huge flat backdrop so the ground reaches the horizon in every
    // direction, even past the edge of the detailed heightmap below.
    DrawPlane((Vector3){0.0f, -0.05f, 0.0f}, (Vector2){50000.0f, 50000.0f}, kGrass);

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

    DrawRain(world.rain, gfx.rainDrops);
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

float GetGroundHeight(const WorldState &world, float worldX, float worldZ) {
    float halfSize = world.worldSize * 0.5f;
    float u = Clamp((worldX + halfSize) / world.worldSize, 0.0f, 1.0f);
    float v = Clamp((worldZ + halfSize) / world.worldSize, 0.0f, 1.0f);

    int i = (int)roundf(u * (world.gridSize - 1));
    int j = (int)roundf(v * (world.gridSize - 1));
    i = (int)Clamp((float)i, 0.0f, (float)(world.gridSize - 1));
    j = (int)Clamp((float)j, 0.0f, (float)(world.gridSize - 1));

    return world.heights[j * world.gridSize + i];
}

ObstacleHit CheckObstacleHit(const WorldState &world, Vector3 planePosition, float planeRadius) {
    ObstacleHit result = ObstacleHit::None;
    for (const Obstacle &o : world.obstacles) {
        if (Vector3Distance(planePosition, o.position) < (o.radius + planeRadius)) {
            if (!o.soft) return ObstacleHit::Hard;
            result = ObstacleHit::Soft;
        }
    }
    return result;
}

void UnloadWorld(WorldState &world) {
    UnloadModel(world.terrainModel);
}
