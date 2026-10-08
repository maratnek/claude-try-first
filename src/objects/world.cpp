#include "world.h"
#include "level_def.h"
#include "render/ground_look.h"
#include "raymath.h"
#include "rlgl.h"
#include <cmath>

namespace {

float SmoothStep01(float t) {
    t = Clamp(t, 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

float ComputeRawHeight(float x, float z, float maxHeight, float flatHalfWidth, float edgeFade, float u, float v) {
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
    h *= SmoothStep01(edgeDist / edgeFade);

    return h;
}

}  // namespace

void GenerateWorld(WorldState &world, const GraphicsSettings &gfx) {
    const TerrainDef &terrain = Level1Def().terrain;
    world.gridSize = terrain.gridSize;
    world.worldSize = terrain.worldSize;
    world.maxHeight = terrain.maxHeight;
    world.flatHalfWidth = terrain.flatHalfWidth;
    world.edgeFade = terrain.edgeFade;
    world.heights.assign(world.gridSize * world.gridSize, 0.0f);

    Image heightImage = GenImageColor(world.gridSize, world.gridSize, BLACK);
    float halfSize = world.worldSize * 0.5f;

    for (int j = 0; j < world.gridSize; j++) {
        for (int i = 0; i < world.gridSize; i++) {
            float u = (float)i / (float)(world.gridSize - 1);
            float v = (float)j / (float)(world.gridSize - 1);
            float x = -halfSize + u * world.worldSize;
            float z = -halfSize + v * world.worldSize;

            float h = ComputeRawHeight(x, z, world.maxHeight, world.flatHalfWidth, world.edgeFade, u, v);
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

    BuildTerrainColors(world, terrainMesh, gfx);
    UploadMesh(&terrainMesh, false);

    world.terrainModel = LoadModelFromMesh(terrainMesh);
    world.terrainModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = WHITE;

    world.obstacles.clear();
    for (const ObstacleDef &o : Level1Def().hardObstacles) AddHardObstacle(world, o.x, o.z, o.height, o.radius);
    for (const ObstacleDef &o : Level1Def().softObstacles) AddSoftObstacle(world, o.x, o.z, o.height, o.radius);

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

void AdvanceWeather(WorldState &world) {
    world.snowRun = !world.snowRun;
}

int ActiveRainDrops(const WorldState &world, const GraphicsSettings &gfx) {
    return (world.snowRun && gfx.snowFlakes > 0) ? 0 : gfx.rainDrops;
}

int ActiveSnowFlakes(const WorldState &world, const GraphicsSettings &gfx) {
    return (world.snowRun || gfx.rainDrops <= 0) ? gfx.snowFlakes : 0;
}
