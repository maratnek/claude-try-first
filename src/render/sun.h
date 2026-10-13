#pragma once
#include "raylib.h"

// Unit vector pointing from the world toward the sun. It sits behind and to the right of the default flight direction,
// so the cast shadow falls ahead-left of the plane where the chase camera can see it; sky disc, plane shading and cast
// shadow all read this one value.
const Vector3 kSunDirection = {0.5f, 0.6f, -0.62f};
