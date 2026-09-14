#pragma once
#include "raylib.h"

// Draws a low-poly glider/plane made of primitives at the given world
// position, facing along +Z rotated by headingDegrees around Y.
void DrawPlaneObject(Vector3 position, float headingDegrees);
