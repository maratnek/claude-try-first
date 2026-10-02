#include "world.h"
#include "raymath.h"
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

}  // namespace

void GenerateWorld(WorldState &world) {
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

    world.terrainModel = LoadModelFromMesh(terrainMesh);
    world.terrainModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = (Color){80, 150, 90, 255};

    world.obstacles.clear();
    float obstacleZs[] = {300.0f, 500.0f, 700.0f, 850.0f};
    float obstacleXs[] = {-8.0f, 10.0f, -6.0f, 9.0f};
    for (int i = 0; i < 4; i++) {
        Obstacle o;
        o.position = (Vector3){obstacleXs[i], GetGroundHeight(world, obstacleXs[i], obstacleZs[i]) + 6.0f, obstacleZs[i]};
        o.radius = 3.0f;
        world.obstacles.push_back(o);
    }
}

void DrawWorldObject(const WorldState &world, const GraphicsSettings &gfx) {
    // Huge flat backdrop so the ground reaches the horizon in every
    // direction, even past the edge of the detailed heightmap below.
    DrawPlane((Vector3){0.0f, -0.05f, 0.0f}, (Vector2){50000.0f, 50000.0f}, (Color){80, 150, 90, 255});

    float halfSize = world.worldSize * 0.5f;
    DrawModel(world.terrainModel, (Vector3){-halfSize, 0.0f, -halfSize}, 1.0f, WHITE);

    if (gfx.distanceMarkers) {
        for (int i = 1; i <= 5; i++) {
            float z = i * 200.0f;
            DrawCylinder((Vector3){-15.0f, GetGroundHeight(world, -15.0f, z), z}, 0.3f, 0.3f, 4.0f, 8, DARKGRAY);
            DrawCylinder((Vector3){15.0f, GetGroundHeight(world, 15.0f, z), z}, 0.3f, 0.3f, 4.0f, 8, DARKGRAY);
        }
    }

    for (const Obstacle &o : world.obstacles) {
        DrawSphere(o.position, o.radius, (Color){180, 30, 30, 255});
        if (gfx.obstacleWires) DrawSphereWires(o.position, o.radius, 8, 8, BLACK);
    }
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

bool CheckObstacleHit(const WorldState &world, Vector3 planePosition, float planeRadius) {
    for (const Obstacle &o : world.obstacles) {
        if (Vector3Distance(planePosition, o.position) < (o.radius + planeRadius)) {
            return true;
        }
    }
    return false;
}

void UnloadWorld(WorldState &world) {
    UnloadModel(world.terrainModel);
}
