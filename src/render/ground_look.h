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

// Generates a tileable grass-noise texture, tiles the terrain UVs over it and applies it per gfx.groundDetail.
void BuildGroundDetail(WorldState &world, Mesh &terrainMesh);

// Swaps the terrain detail texture on/off to match gfx.groundDetail; no-op when already current.
void ApplyGroundDetail(WorldState &world, const GraphicsSettings &gfx);
