#include "ground_look.h"
#include "raymath.h"
#include <cmath>

namespace {

float SmoothStep01(float t) {
    t = Clamp(t, 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

Color LerpColor(Color a, Color b, float t) {
    return (Color){(unsigned char)(a.r + (b.r - a.r) * t), (unsigned char)(a.g + (b.g - a.g) * t),
                   (unsigned char)(a.b + (b.b - a.b) * t), 255};
}

// Flat normal shades to exactly 1 so flat low ground matches the backdrop plane colour.
Color TerrainVertexColor(float heightNorm, Vector3 normal) {
    const Color dirt = {146, 120, 82, 255};
    const Color rock = {128, 126, 132, 255};
    const Vector3 light = Vector3Normalize((Vector3){0.5f, 0.8f, 0.3f});

    float slope = 1.0f - Clamp(normal.y, 0.0f, 1.0f);
    float dirtW = fmaxf(0.85f * SmoothStep01((heightNorm - 0.55f) / 0.4f), SmoothStep01((slope - 0.025f) / 0.06f));
    float rockW = fmaxf(SmoothStep01((heightNorm - 0.88f) / 0.12f), SmoothStep01((slope - 0.09f) / 0.08f));

    Color c = LerpColor(LerpColor(kGroundGrass, dirt, dirtW), rock, rockW);
    float shade = Clamp(0.7f + 0.3f * Vector3DotProduct(normal, light) / light.y, 0.6f, 1.1f);
    c.r = (unsigned char)fminf(c.r * shade, 255.0f);
    c.g = (unsigned char)fminf(c.g * shade, 255.0f);
    c.b = (unsigned char)fminf(c.b * shade, 255.0f);
    return c;
}

}  // namespace

const Color kGroundGrass = {80, 150, 90, 255};

void BuildTerrainColors(WorldState &world, Mesh &mesh, const GraphicsSettings &gfx) {
    int colorBytes = mesh.vertexCount * 4;
    world.terrainShadedColors.assign(colorBytes, 255);
    world.terrainFlatColors.assign(colorBytes, 255);
    for (int v = 0; v < mesh.vertexCount; v++) {
        Vector3 n = {mesh.normals[v * 3], mesh.normals[v * 3 + 1], mesh.normals[v * 3 + 2]};
        Color c = TerrainVertexColor(mesh.vertices[v * 3 + 1] / world.maxHeight, n);
        world.terrainShadedColors[v * 4] = c.r;
        world.terrainShadedColors[v * 4 + 1] = c.g;
        world.terrainShadedColors[v * 4 + 2] = c.b;
        world.terrainFlatColors[v * 4] = kGroundGrass.r;
        world.terrainFlatColors[v * 4 + 1] = kGroundGrass.g;
        world.terrainFlatColors[v * 4 + 2] = kGroundGrass.b;
    }
    const std::vector<unsigned char> &initial = gfx.terrainColors ? world.terrainShadedColors : world.terrainFlatColors;
    world.terrainColored = gfx.terrainColors;
    mesh.colors = (unsigned char *)MemAlloc(colorBytes);
    for (int i = 0; i < colorBytes; i++) mesh.colors[i] = initial[i];
}

void ApplyTerrainColors(WorldState &world, const GraphicsSettings &gfx) {
    if (world.terrainColored == gfx.terrainColors) return;
    const std::vector<unsigned char> &src = gfx.terrainColors ? world.terrainShadedColors : world.terrainFlatColors;
    UpdateMeshBuffer(world.terrainModel.meshes[0], 3, src.data(), (int)src.size(), 0);
    world.terrainColored = gfx.terrainColors;
}

void DrawGroundBackdrop() {
    DrawPlane((Vector3){0.0f, -0.05f, 0.0f}, (Vector2){50000.0f, 50000.0f}, kGroundGrass);
}
