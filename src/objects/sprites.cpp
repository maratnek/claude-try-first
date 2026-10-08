#include "sprites.h"
#include "raymath.h"
#include "rlgl.h"
#include <cmath>

namespace {
Texture2D gPuff{};
Texture2D gFlake{};
bool gAdditive = false;

Texture2D MakeRadial(int size, float density) {
    Image image = GenImageGradientRadial(size, size, density, WHITE, BLANK);
    Texture2D texture = LoadTextureFromImage(image);
    UnloadImage(image);
    SetTextureFilter(texture, TEXTURE_FILTER_BILINEAR);
    return texture;
}

// Created lazily so no call site needs an extra init step; the GL context exists by the first draw.
const Texture2D &SpriteTexture(SpriteKind kind) {
    if (kind == SpriteKind::Puff) {
        if (gPuff.id == 0) gPuff = MakeRadial(64, 0.0f);
        return gPuff;
    }
    if (gFlake.id == 0) gFlake = MakeRadial(32, 0.45f);
    return gFlake;
}
}  // namespace

BillboardAxes CurrentBillboardAxes() {
    Matrix view = rlGetMatrixModelview();
    return {{view.m0, view.m4, view.m8}, {view.m1, view.m5, view.m9}};
}

void BeginSprites(SpriteKind kind, bool additive) {
    rlDrawRenderBatchActive();
    rlDisableDepthMask();
    rlDisableBackfaceCulling();
    gAdditive = additive;
    if (additive) BeginBlendMode(BLEND_ADDITIVE);
    rlSetTexture(SpriteTexture(kind).id);
    rlBegin(RL_QUADS);
}

void DrawSprite(const BillboardAxes &axes, Vector3 centre, float halfSize, float rotation, Color color) {
    rlCheckRenderBatchLimit(4);
    float c = cosf(rotation) * halfSize, s = sinf(rotation) * halfSize;
    Vector3 r = Vector3Add(Vector3Scale(axes.right, c), Vector3Scale(axes.up, s));
    Vector3 u = Vector3Add(Vector3Scale(axes.up, c), Vector3Scale(axes.right, -s));
    rlColor4ub(color.r, color.g, color.b, color.a);
    rlTexCoord2f(0.0f, 1.0f);
    rlVertex3f(centre.x - r.x - u.x, centre.y - r.y - u.y, centre.z - r.z - u.z);
    rlTexCoord2f(1.0f, 1.0f);
    rlVertex3f(centre.x + r.x - u.x, centre.y + r.y - u.y, centre.z + r.z - u.z);
    rlTexCoord2f(1.0f, 0.0f);
    rlVertex3f(centre.x + r.x + u.x, centre.y + r.y + u.y, centre.z + r.z + u.z);
    rlTexCoord2f(0.0f, 0.0f);
    rlVertex3f(centre.x - r.x + u.x, centre.y - r.y + u.y, centre.z - r.z + u.z);
}

void EndSprites() {
    rlEnd();
    rlSetTexture(0);
    rlDrawRenderBatchActive();
    if (gAdditive) EndBlendMode();
    rlEnableBackfaceCulling();
    rlEnableDepthMask();
}

void UnloadSprites() {
    if (gPuff.id != 0) UnloadTexture(gPuff);
    if (gFlake.id != 0) UnloadTexture(gFlake);
    gPuff = Texture2D{};
    gFlake = Texture2D{};
}
