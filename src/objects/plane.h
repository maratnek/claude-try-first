#pragma once
#include "raylib.h"

// Draws a low-poly glider/plane made of primitives at the given world
// position and orientation in degrees (matching PlaneState's convention:
// positive pitch = nose up, positive roll = banking right).
void DrawPlaneObject(Vector3 position, float yawDegrees, float pitchDegrees, float rollDegrees);
