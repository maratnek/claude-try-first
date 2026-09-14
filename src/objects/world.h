#pragma once
#include "raylib.h"
#include <vector>

struct Obstacle {
    Vector3 position;
    float radius;
};

struct WorldState {
    int gridSize = 64;
    float worldSize = 1400.0f;
    float maxHeight = 10.0f;
    float flatHalfWidth = 40.0f;  // flight corridor kept flat around x=0
    std::vector<float> heights;   // gridSize * gridSize, row-major
    Model terrainModel{};
    std::vector<Obstacle> obstacles;
};

// Builds the terrain heightmap and obstacle layout. Call once at startup.
void GenerateWorld(WorldState &world);

// Draws terrain, distance-marker pillars, and obstacles.
void DrawWorldObject(const WorldState &world);

// Ground height (meters) at the given world X/Z, bilinear-free nearest sample.
float GetGroundHeight(const WorldState &world, float worldX, float worldZ);

// True if planePosition is within radius of any obstacle.
bool CheckObstacleHit(const WorldState &world, Vector3 planePosition, float planeRadius);

void UnloadWorld(WorldState &world);
