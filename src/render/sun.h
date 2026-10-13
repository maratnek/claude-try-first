#pragma once
#include "raylib.h"

// Behind and to the left of the default flight direction (+z forward, +x is the plane's left), so the shadow falls
// ahead-right where the chase camera sees it.
const Vector3 kSunDirection = {0.5f, 0.6f, -0.62f};
