#pragma once
#include "raylib.h"

// Draws a minimal low-poly humanoid (cylinder body + sphere head) at the
// given ground position, facing along +Z rotated by headingDegrees around Y.
void DrawCharacterObject(Vector3 position, float headingDegrees, Color bodyColor);
