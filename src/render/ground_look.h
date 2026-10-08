#pragma once
#include "raylib.h"
#include "objects/world.h"
#include "settings.h"

extern const Color kGroundGrass;

// Fills world.terrainShadedColors/terrainFlatColors for every vertex and sets mesh.colors to the set matching gfx.terrainColors.
void BuildTerrainColors(WorldState &world, Mesh &terrainMesh, const GraphicsSettings &gfx);

// Swaps the terrain vertex colours to match gfx.terrainColors; no-op when already current.
void ApplyTerrainColors(WorldState &world, const GraphicsSettings &gfx);

// Huge flat backdrop so the ground reaches the horizon in every direction, even past the edge of the detailed heightmap.
void DrawGroundBackdrop();
