#pragma once
#include "raylib.h"
#include "settings.h"

// Clears the frame and paints the sky; call at the start of the 3D frame, before BeginMode3D.
void DrawSky(const Camera3D &camera, const GraphicsSettings &gfx);

// Soft horizon-coloured band over distant terrain; call after the world is drawn and before the plane, outside BeginMode3D.
void DrawHorizonHaze(const Camera3D &camera, const GraphicsSettings &gfx);
