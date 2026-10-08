#pragma once
#include "raylib.h"
#include "objects/world.h"
#include "settings.h"

// Draws terrain, distance-marker pillars, obstacles, decorative trees/rocks and clouds near viewPosition (density from gfx.scatterDensity), and the run's weather: rain (gfx.rainDrops) or snow (gfx.snowFlakes), never both.
void DrawWorldObject(const WorldState &world, const GraphicsSettings &gfx, Vector3 viewPosition);

// Flat translucent plane-shaped shadow on the terrain below the plane; hidden above a max altitude.
void DrawBlobShadow(const WorldState &world, Vector3 planePosition, float yawDegrees);
