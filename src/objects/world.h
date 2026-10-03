#pragma once
#include "raylib.h"
#include "settings.h"
#include "scatter.h"
#include <vector>

struct Obstacle {
    Vector3 position;
    float radius;
    bool soft = false;
};

enum class ObstacleHit { None, Soft, Hard };

struct WorldState {
    int gridSize = 64;
    float worldSize = 1400.0f;
    float maxHeight = 10.0f;
    float flatHalfWidth = 40.0f;  // flight corridor kept flat around x=0
    std::vector<float> heights;   // gridSize * gridSize, row-major
    Model terrainModel{};
    std::vector<unsigned char> terrainShadedColors;  // RGBA per vertex
    std::vector<unsigned char> terrainFlatColors;
    bool terrainColored = false;
    std::vector<Obstacle> obstacles;
    ScatterState scatter;
};

// Builds the terrain heightmap and obstacle layout. Call once at startup.
void GenerateWorld(WorldState &world, const GraphicsSettings &gfx);

// Swaps the terrain vertex colours to match gfx.terrainColors; no-op when already current.
void ApplyTerrainColors(WorldState &world, const GraphicsSettings &gfx);

// Draws terrain, distance-marker pillars, obstacles, and decorative trees/rocks near viewPosition (density from gfx.scatterDensity).
void DrawWorldObject(const WorldState &world, const GraphicsSettings &gfx, Vector3 viewPosition);

// Flat translucent plane-shaped shadow on the terrain below the plane; hidden above a max altitude.
void DrawBlobShadow(const WorldState &world, Vector3 planePosition, float yawDegrees);

// Ground height (meters) at the given world X/Z, bilinear-free nearest sample.
float GetGroundHeight(const WorldState &world, float worldX, float worldZ);

// Adds a hard obstacle (crashes the plane) centered height meters above the ground.
void AddHardObstacle(WorldState &world, float x, float z, float height, float radius);

// Adds a soft obstacle (bush/treetop canopy; damages the plane) centered height meters above the ground.
void AddSoftObstacle(WorldState &world, float x, float z, float height, float radius);

// Hard wins over soft when the plane overlaps both.
ObstacleHit CheckObstacleHit(const WorldState &world, Vector3 planePosition, float planeRadius);

void UnloadWorld(WorldState &world);
