#pragma once
#include "raylib.h"
#include "settings.h"

// Clears the frame and paints the sky; call at the start of the 3D frame, before BeginMode3D.
void DrawSky(const Camera3D &camera, const GraphicsSettings &gfx);
