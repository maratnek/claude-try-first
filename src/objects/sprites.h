#pragma once
#include "raylib.h"

// Soft round particle sprites generated in code (no asset files).
enum class SpriteKind { Puff, Flake };

// Camera-facing axes for billboards, from the current 3D modelview matrix.
struct BillboardAxes {
    Vector3 right;
    Vector3 up;
};
BillboardAxes CurrentBillboardAxes();

// Opens a batch of textured quads; call between BeginMode3D/EndMode3D.
void BeginSprites(SpriteKind kind, bool additive);
// halfSize in world units; rotation in radians around the view axis.
void DrawSprite(const BillboardAxes &axes, Vector3 centre, float halfSize, float rotation, Color color);
void EndSprites();

void UnloadSprites();
