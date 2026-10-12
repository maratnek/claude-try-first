#include "sky.h"
#include <cmath>

namespace {

const Color kSkyZenith = {58, 120, 200, 255};
const Color kSkyHorizon = {190, 220, 240, 255};
const Vector3 kSunDir = {0.25f, 0.08f, 0.96f};
const float kHazeAbove = 0.06f;
const float kHazeBelow = 0.14f;
const unsigned char kHazeAlpha = 150;
const Color kSunCore = {255, 250, 225, 255};
const Color kSunGlowInner = {255, 240, 190, 150};
const Color kSunGlowOuter = {255, 240, 190, 0};

void DrawSun(const Camera3D &camera) {
    Vector3 view = {camera.target.x - camera.position.x, camera.target.y - camera.position.y, camera.target.z - camera.position.z};
    float viewLen = std::sqrt(view.x * view.x + view.y * view.y + view.z * view.z);
    float facing = (view.x * kSunDir.x + view.y * kSunDir.y + view.z * kSunDir.z) / viewLen;
    // Behind or far to the side of the camera the projected point is meaningless.
    if (facing < 0.2f) return;

    Vector3 point = {camera.position.x + kSunDir.x * 20000.0f, camera.position.y + kSunDir.y * 20000.0f,
                     camera.position.z + kSunDir.z * 20000.0f};
    Vector2 screen = GetWorldToScreen(point, camera);
    float unit = static_cast<float>(GetScreenHeight()) / 100.0f;
    int x = static_cast<int>(screen.x);
    int y = static_cast<int>(screen.y);
    DrawCircleGradient(x, y, unit * 18.0f, kSunGlowInner, kSunGlowOuter);
    DrawCircle(x, y, unit * 3.2f, kSunCore);
}

int HorizonScreenY(const Camera3D &camera) {
    Vector3 forward = {camera.target.x - camera.position.x, 0.0f, camera.target.z - camera.position.z};
    float len = std::sqrt(forward.x * forward.x + forward.z * forward.z);
    forward = {forward.x / len, 0.0f, forward.z / len};
    // GetWorldToScreen does no clipping, so a point far beyond the far plane still lands on the horizon line.
    Vector3 far = {camera.position.x + forward.x * 20000.0f, camera.position.y, camera.position.z + forward.z * 20000.0f};
    return static_cast<int>(GetWorldToScreen(far, camera).y);
}

}  // namespace

void DrawSky(const Camera3D &camera, const GraphicsSettings &gfx) {
    ClearBackground(SKYBLUE);
    if (!gfx.skyGradient) {
        if (gfx.sunDisc) DrawSun(camera);
        return;
    }

    int horizonY = HorizonScreenY(camera);
    int w = GetScreenWidth();
    int h = GetScreenHeight();
    if (horizonY > 0) {
        int top = horizonY < h ? horizonY : h;
        DrawRectangleGradientV(0, 0, w, top, kSkyZenith, kSkyHorizon);
    }
    if (horizonY < h) {
        int from = horizonY > 0 ? horizonY : 0;
        DrawRectangle(0, from, w, h - from, kSkyHorizon);
    }
    if (gfx.sunDisc) DrawSun(camera);
}

void DrawHorizonHaze(const Camera3D &camera, const GraphicsSettings &gfx) {
    if (!gfx.horizonHaze) return;
    int w = GetScreenWidth();
    int h = GetScreenHeight();
    int horizonY = HorizonScreenY(camera);
    int above = static_cast<int>(h * kHazeAbove);
    int below = static_cast<int>(h * kHazeBelow);
    if (horizonY + below < 0 || horizonY - above > h) return;
    Color clear = {kSkyHorizon.r, kSkyHorizon.g, kSkyHorizon.b, 0};
    Color dense = {kSkyHorizon.r, kSkyHorizon.g, kSkyHorizon.b, kHazeAlpha};
    DrawRectangleGradientV(0, horizonY - above, w, above, clear, dense);
    DrawRectangleGradientV(0, horizonY, w, below, dense, clear);
}
