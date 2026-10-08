#include "touch_layout.h"
#include <cstdio>

static int failures = 0;

static void Check(bool ok, const char *what) {
    if (!ok) {
        std::printf("FAIL: %s\n", what);
        failures++;
    }
}

static bool Overlaps(const TouchBox &a, const TouchBox &b) {
    return a.x < b.x + b.w && b.x < a.x + a.w && a.y < b.y + b.h && b.y < a.y + a.h;
}

int main() {
    const float screens[] = {1280.0f, 844.0f, 568.0f};
    for (float w : screens) {
        SettingsButtons b = LayoutSettingsButtons(w, 47.0f, 0.0f);
        Check(!Overlaps(b.gfx, b.volDown) && !Overlaps(b.volDown, b.volUp) && !Overlaps(b.gfx, b.volUp), "buttons do not overlap");
        Check(b.volUp.x + b.volUp.w <= w - 47.0f, "inside right safe area");
        Check(b.gfx.y >= 8.0f + 36.0f, "below web fullscreen button");
        Check(b.gfx.y + b.gfx.h <= 85.0f, "above volume readout");
        Check(TouchBoxContains(b.volUp, b.volUp.x + 1.0f, b.volUp.y + 1.0f), "contains inside point");
        Check(!TouchBoxContains(b.volUp, b.volUp.x + b.volUp.w, b.volUp.y), "excludes far edge");
        Check(!TouchBoxContains(b.gfx, b.volDown.x + 1.0f, b.volDown.y + 1.0f), "gfx does not claim volDown");
    }
    return failures == 0 ? 0 : 1;
}
