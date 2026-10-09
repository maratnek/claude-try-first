#pragma once
#include "raylib.h"
#include "settings.h"
#include "scatter.h"
#include "clouds.h"
#include "rain.h"
#include "snow.h"
#include <vector>

struct Obstacle {
    Vector3 position;  // soft: canopy centre; tree: base of the trunk on the ground
    float radius;      // soft: canopy radius; tree: crown radius at its widest
    float height = 0.0f;  // tree only: total height
    bool soft = false;
};

enum class ObstacleHit { None, Soft, Hard };

struct WorldState {
    int gridSize = 0;
    float worldSize = 0.0f;
    float maxHeight = 0.0f;
    float flatHalfWidth = 0.0f;  // flight corridor kept flat around x=0
    float edgeFade = 0.0f;
    std::vector<float> heights;   // gridSize * gridSize, row-major
    Model terrainModel{};
    std::vector<unsigned char> terrainShadedColors;  // RGBA per vertex
    std::vector<unsigned char> terrainFlatColors;
    bool terrainColored = false;
    std::vector<Obstacle> obstacles;
    ScatterState scatter;
    CloudsState clouds;
    RainState rain;
    SnowState snow;
    bool snowRun = false;
};

// Builds the terrain heightmap (shape from the level definition) and obstacle layout. Call once at startup.
void GenerateWorld(WorldState &world, const GraphicsSettings &gfx);

// Swaps the terrain vertex colours to match gfx.terrainColors; no-op when already current.
void ApplyTerrainColors(WorldState &world, const GraphicsSettings &gfx);

// Draws terrain, distance-marker pillars, obstacles, decorative trees/rocks and clouds near viewPosition (density from gfx.scatterDensity), and the run's weather: rain (gfx.rainDrops) or snow (gfx.snowFlakes), never both.
void DrawWorldObject(const WorldState &world, const GraphicsSettings &gfx, Vector3 viewPosition);

// Flat translucent plane-shaped shadow on the terrain below the plane; hidden above a max altitude.
void DrawBlobShadow(const WorldState &world, Vector3 planePosition, float yawDegrees);

// Ground height (meters) at the given world X/Z, bilinear-free nearest sample.
float GetGroundHeight(const WorldState &world, float worldX, float worldZ);

// Adds a hard obstacle (crashes the plane): a tall pine, height meters tall with a crown radius of radius meters.
void AddHardObstacle(WorldState &world, float x, float z, float height, float radius);

// Adds a soft obstacle (bush/treetop canopy; damages the plane) centered height meters above the ground.
void AddSoftObstacle(WorldState &world, float x, float z, float height, float radius);

// Hard wins over soft when the plane overlaps both.
ObstacleHit CheckObstacleHit(const WorldState &world, Vector3 planePosition, float planeRadius);

// Rain and snow are mutually exclusive: when both counts are > 0 the run's weather alternates (see AdvanceWeather); a count of 0 disables that kind.
void AdvanceWeather(WorldState &world);
int ActiveRainDrops(const WorldState &world, const GraphicsSettings &gfx);
int ActiveSnowFlakes(const WorldState &world, const GraphicsSettings &gfx);

void UnloadWorld(WorldState &world);
