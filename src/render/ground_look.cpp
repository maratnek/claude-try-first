#include "ground_look.h"
#include "raymath.h"
#include "rlgl.h"
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

float Hash2(int x, int y, int seed) {
    unsigned h = (unsigned)x * 374761393u + (unsigned)y * 668265263u + (unsigned)seed * 2246822519u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return ((h ^ (h >> 16)) & 0xffff) / 65535.0f;
}

// Value noise that wraps every `period` cells, so the texture tiles without seams.
float TileNoise(float x, float y, int period, int seed) {
    int xi = (int)floorf(x), yi = (int)floorf(y);
    float fx = SmoothStep01(x - xi), fy = SmoothStep01(y - yi);
    auto at = [&](int ix, int iy) { return Hash2(((ix % period) + period) % period, ((iy % period) + period) % period, seed); };
    float a = at(xi, yi) + (at(xi + 1, yi) - at(xi, yi)) * fx;
    float b = at(xi, yi + 1) + (at(xi + 1, yi + 1) - at(xi, yi + 1)) * fx;
    return a + (b - a) * fy;
}

const int kDetailSize = 128;
const float kDetailTileMetres = 24.0f;

// Mid-grey-ish values: the texture multiplies the vertex colour, so it only darkens/varies and never changes the base hue.
Texture2D MakeGroundDetailTexture() {
    Image img = GenImageColor(kDetailSize, kDetailSize, WHITE);
    Color *px = (Color *)img.data;
    for (int y = 0; y < kDetailSize; y++) {
        for (int x = 0; x < kDetailSize; x++) {
            float u = (float)x / kDetailSize, v = (float)y / kDetailSize;
            float patch = TileNoise(u * 4, v * 4, 4, 1) * 0.6f + TileNoise(u * 8, v * 8, 8, 2) * 0.4f;
            float blades = TileNoise(u * 64, v * 16, 16, 3);
            float speck = Hash2(x, y, 7);
            float shade = 0.78f + 0.14f * patch + 0.08f * blades + 0.05f * speck;
            if (speck > 0.96f) shade *= 0.88f;
            unsigned char g = (unsigned char)(Clamp(shade, 0.0f, 1.0f) * 255.0f);
            px[y * kDetailSize + x] = (Color){g, g, g, 255};
        }
    }
    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);
    GenTextureMipmaps(&tex);
    SetTextureFilter(tex, TEXTURE_FILTER_TRILINEAR);
    SetTextureWrap(tex, TEXTURE_WRAP_REPEAT);
    return tex;
}

}  // namespace

const Color kGroundGrass = {80, 150, 90, 255};

namespace {

int TerrainColorMode(const GraphicsSettings &gfx) {
    if (!gfx.terrainColors) return 0;
    return gfx.terrainRelief ? 2 : 1;
}

const std::vector<unsigned char> &TerrainColorSet(const WorldState &world, int mode) {
    return mode == 0 ? world.terrainFlatColors : (mode == 1 ? world.terrainShadedColors : world.terrainReliefColors);
}

// Positive on ridges, negative in valleys; flat ground and constant slopes give 0, so the flight corridor keeps its colour.
float TerrainCurvature(const WorldState &world, float x, float z) {
    float step = world.worldSize / (float)(world.gridSize - 1) * 3.0f;
    float around = GetGroundHeight(world, x + step, z) + GetGroundHeight(world, x - step, z) +
                   GetGroundHeight(world, x, z + step) + GetGroundHeight(world, x, z - step);
    return (GetGroundHeight(world, x, z) * 4.0f - around) / world.maxHeight;
}

Color ReliefTint(Color c, float curvature) {
    const Color valley = {58, 104, 96, 255};
    const Color ridge = {206, 190, 110, 255};
    float t = Clamp(curvature * 6.0f, -1.0f, 1.0f);
    Color shaded = LerpColor(c, t < 0.0f ? valley : ridge, fabsf(t) * (t < 0.0f ? 0.5f : 0.35f));
    float dark = t < 0.0f ? 1.0f + 0.25f * t : 1.0f;
    shaded.r = (unsigned char)(shaded.r * dark);
    shaded.g = (unsigned char)(shaded.g * dark);
    shaded.b = (unsigned char)(shaded.b * dark);
    return shaded;
}

}  // namespace

void BuildTerrainColors(WorldState &world, Mesh &mesh, const GraphicsSettings &gfx) {
    int colorBytes = mesh.vertexCount * 4;
    world.terrainShadedColors.assign(colorBytes, 255);
    world.terrainReliefColors.assign(colorBytes, 255);
    world.terrainFlatColors.assign(colorBytes, 255);
    for (int v = 0; v < mesh.vertexCount; v++) {
        Vector3 n = {mesh.normals[v * 3], mesh.normals[v * 3 + 1], mesh.normals[v * 3 + 2]};
        Color c = TerrainVertexColor(mesh.vertices[v * 3 + 1] / world.maxHeight, n);
        Color r = ReliefTint(c, TerrainCurvature(world, mesh.vertices[v * 3], mesh.vertices[v * 3 + 2]));
        unsigned char *shaded = &world.terrainShadedColors[v * 4];
        unsigned char *relief = &world.terrainReliefColors[v * 4];
        unsigned char *flat = &world.terrainFlatColors[v * 4];
        shaded[0] = c.r; shaded[1] = c.g; shaded[2] = c.b;
        relief[0] = r.r; relief[1] = r.g; relief[2] = r.b;
        flat[0] = kGroundGrass.r; flat[1] = kGroundGrass.g; flat[2] = kGroundGrass.b;
    }
    int mode = TerrainColorMode(gfx);
    const std::vector<unsigned char> &initial = TerrainColorSet(world, mode);
    world.terrainColorMode = mode;
    mesh.colors = (unsigned char *)MemAlloc(colorBytes);
    for (int i = 0; i < colorBytes; i++) mesh.colors[i] = initial[i];
}

void ApplyTerrainColors(WorldState &world, const GraphicsSettings &gfx) {
    int mode = TerrainColorMode(gfx);
    if (world.terrainColorMode == mode) return;
    const std::vector<unsigned char> &src = TerrainColorSet(world, mode);
    UpdateMeshBuffer(world.terrainModel.meshes[0], 3, src.data(), (int)src.size(), 0);
    world.terrainColorMode = mode;
}

void DrawGroundBackdrop() {
    DrawPlane((Vector3){0.0f, -0.05f, 0.0f}, (Vector2){50000.0f, 50000.0f}, kGroundGrass);
}

void BuildGroundDetail(WorldState &world, Mesh &terrainMesh) {
    float repeats = world.worldSize / kDetailTileMetres;
    for (int i = 0; i < terrainMesh.vertexCount * 2; i++) terrainMesh.texcoords[i] *= repeats;
    world.groundDetail = MakeGroundDetailTexture();
    world.groundDetailOn = false;
}

void ApplyGroundDetail(WorldState &world, const GraphicsSettings &gfx) {
    if (world.groundDetailOn == gfx.groundDetail || world.groundDetail.id == 0) return;
    Texture2D tex = gfx.groundDetail ? world.groundDetail : (Texture2D){rlGetTextureIdDefault(), 1, 1, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8};
    world.terrainModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = tex;
    world.groundDetailOn = gfx.groundDetail;
}
