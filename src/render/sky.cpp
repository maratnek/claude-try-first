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
    float len = sqrtf(forward.x * forward.x + forward.z * forward.z);
    if (len < 1e-4f) forward = {0.0f, 0.0f, 1.0f};
    else forward = {forward.x / len, 0.0f, forward.z / len};
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
