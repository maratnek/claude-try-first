#pragma once
#include "raylib.h"
#include "objects/scatter.h"
#include "settings.h"

// Builds the tree texture and the alpha-cutout shader; call once after the window exists.
void LoadTreeLook();
void UnloadTreeLook();

// Crossed textured cards (cartoon canopy with outline) for every tree prop, same density/distance rules as DrawScatter. No-op if the shader failed to load.
void DrawTreeCards(const ScatterState &scatter, float density, Vector3 viewPosition, float drawDistance);

bool TreeCardsAvailable();
