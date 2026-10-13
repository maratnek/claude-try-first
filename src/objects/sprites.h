#pragma once
#include "raylib.h"

// Soft round particle sprites generated in code (no asset files).
enum class SpriteKind { Puff, Flake, Streak };

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
// Streak kind only: a quad from tail to head, turned to face the camera, tapering to a point-like tail;
// per-end colours let alpha fade along the length. width is full width in world units at the head.
void DrawStreakSprite(const BillboardAxes &axes, Vector3 tail, Vector3 head, float width, Color tailColor, Color headColor);
void EndSprites();

void UnloadSprites();
