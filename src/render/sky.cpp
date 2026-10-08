#include "sky.h"
#include <cmath>

namespace {

const Color kSkyZenith = {58, 120, 200, 255};
const Color kSkyHorizon = {190, 220, 240, 255};

}  // namespace

void DrawSky(const Camera3D &camera, const GraphicsSettings &gfx) {
    ClearBackground(SKYBLUE);
    if (!gfx.skyGradient) return;

    Vector3 forward = {camera.target.x - camera.position.x, 0.0f, camera.target.z - camera.position.z};
    float len = std::sqrt(forward.x * forward.x + forward.z * forward.z);
    forward = {forward.x / len, 0.0f, forward.z / len};
    // GetWorldToScreen does no clipping, so a point far beyond the far plane still lands on the horizon line.
    Vector3 far = {camera.position.x + forward.x * 20000.0f, camera.position.y, camera.position.z + forward.z * 20000.0f};
    int horizonY = static_cast<int>(GetWorldToScreen(far, camera).y);

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
}
