#include "touch_layout.h"

SettingsButtons LayoutSettingsButtons(float screenWidth, float safeRight, float safeTop) {
    const float gap = 8.0f, h = 32.0f, gfxW = 150.0f, volW = 56.0f, margin = 10.0f;
    const float y = safeTop + 48.0f;
    float x = screenWidth - safeRight - margin - (gfxW + 2.0f * volW + 2.0f * gap);
    SettingsButtons b;
    b.gfx = {x, y, gfxW, h};
    x += gfxW + gap;
    b.volDown = {x, y, volW, h};
    x += volW + gap;
    b.volUp = {x, y, volW, h};
    return b;
}

bool TouchBoxContains(const TouchBox &box, float px, float py) {
    return px >= box.x && px < box.x + box.w && py >= box.y && py < box.y + box.h;
}
