#include "sprites.h"
#include "raymath.h"
#include "rlgl.h"
#include <cmath>

namespace {
Texture2D gPuff{};
Texture2D gFlake{};
Texture2D gStreak{};
bool gAdditive = false;

Texture2D MakeRadial(int size, float density) {
    Image image = GenImageGradientRadial(size, size, density, WHITE, BLANK);
    Texture2D texture = LoadTextureFromImage(image);
    UnloadImage(image);
    SetTextureFilter(texture, TEXTURE_FILTER_BILINEAR);
    return texture;
}

Texture2D MakeStreak(int width, int height) {
    Image image = GenImageColor(width, height, BLANK);
    for (int x = 0; x < width; x++) {
        float across = 1.0f - fabsf(((float)x + 0.5f) / (float)width * 2.0f - 1.0f);
        unsigned char alpha = (unsigned char)(255.0f * across * sqrtf(across));
        for (int y = 0; y < height; y++) ImageDrawPixel(&image, x, y, {255, 255, 255, alpha});
    }
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
    if (kind == SpriteKind::Streak) {
        if (gStreak.id == 0) gStreak = MakeStreak(16, 4);
        return gStreak;
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

void DrawStreakSprite(const BillboardAxes &axes, Vector3 tail, Vector3 head, float width, Color tailColor, Color headColor) {
    Vector3 along = Vector3Subtract(head, tail);
    Vector3 toward = Vector3CrossProduct(axes.right, axes.up);
    Vector3 side = Vector3CrossProduct(along, toward);
    float len = Vector3Length(side);
    if (len < 1e-6f) return;
    side = Vector3Scale(side, 1.0f / len);
    Vector3 h = Vector3Scale(side, 0.5f * width);
    Vector3 t = Vector3Scale(side, 0.15f * width);
    rlCheckRenderBatchLimit(4);
    rlColor4ub(tailColor.r, tailColor.g, tailColor.b, tailColor.a);
    rlTexCoord2f(0.0f, 1.0f);
    rlVertex3f(tail.x - t.x, tail.y - t.y, tail.z - t.z);
    rlColor4ub(headColor.r, headColor.g, headColor.b, headColor.a);
    rlTexCoord2f(0.0f, 0.0f);
    rlVertex3f(head.x - h.x, head.y - h.y, head.z - h.z);
    rlTexCoord2f(1.0f, 0.0f);
    rlVertex3f(head.x + h.x, head.y + h.y, head.z + h.z);
    rlColor4ub(tailColor.r, tailColor.g, tailColor.b, tailColor.a);
    rlTexCoord2f(1.0f, 1.0f);
    rlVertex3f(tail.x + t.x, tail.y + t.y, tail.z + t.z);
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
    if (gStreak.id != 0) UnloadTexture(gStreak);
    gPuff = Texture2D{};
    gFlake = Texture2D{};
    gStreak = Texture2D{};
}
